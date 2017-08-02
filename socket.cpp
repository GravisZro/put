#include "socket.h"

// PDTK
#include <cxxutils/error_helpers.h>
#include <cxxutils/colors.h>

static_assert(CMSG_SPACE(sizeof(int)) <= 64, "seriously?");

namespace posix
{
  inline bool peercred(fd_t socket, proccred_t& cred) noexcept
    { return ::peercred(socket, cred) == posix::success_response; }
}

GenericSocket::GenericSocket(EDomain   domain,
                             EType     type,
                             EProtocol protocol,
                             int       flags) noexcept
  : GenericSocket(posix::socket(domain, type, protocol, flags)) { }

GenericSocket::GenericSocket(posix::fd_t socket) noexcept
  : m_connected(false), m_socket(socket)
{
  Object::connect(m_socket, EventFlags::Readable, this, &GenericSocket::read); // monitor socket connection
}

GenericSocket::~GenericSocket(void) noexcept { disconnect(); }

void GenericSocket::disconnect(void) noexcept
{
  if(m_selfaddr != EDomain::unspec)
  {
    ::unlink(m_selfaddr.sun_path);
    m_selfaddr = EDomain::unspec;
  }

  if(m_socket != posix::invalid_descriptor)
  {
    Object::enqueue(disconnected, m_socket);
    Object::disconnect(m_socket, EventFlags::Readable);
    posix::close(m_socket); // connection severed!
    m_socket = posix::invalid_descriptor;
  }

  m_connected = false;
}


bool ClientSocket::connect(const char *socket_path) noexcept
{
  flaw(m_connected, posix::warning,,false,
       "Client socket is already connected!")

  flaw(std::strlen(socket_path) >= sizeof(sockaddr_un::sun_path),posix::warning,,false,
       "socket_path exceeds the maximum path length, %lu bytes", sizeof(sockaddr_un::sun_path))

  posix::sockaddr_t peeraddr;
  proccred_t peercred;

  peeraddr = socket_path;
  peeraddr = EDomain::local;
  m_selfaddr = EDomain::unspec;

  flaw(!posix::connect(m_socket, peeraddr, peeraddr.size()), posix::warning,, false,
       "connect() failure: %s", std::strerror(errno)) // connect to peer process
  m_connected = true;

  flaw(!posix::peercred(m_socket, peercred), posix::warning,, false,
       "peercred() failure: %s", std::strerror(errno)) // get creditials of connected peer process

  Object::enqueue(connected, m_socket, peeraddr, peercred);
  return true;
}

bool ClientSocket::write(const vfifo& buffer, posix::fd_t fd) const noexcept
{
  msghdr header = {};
  iovec iov = {};
  char aux_buffer[64] = { 0 };

  header.msg_iov = &iov;
  header.msg_iovlen = 1;
  header.msg_control = aux_buffer;

  iov.iov_base = buffer.begin();
  iov.iov_len = buffer.size();

  header.msg_controllen = 0;

  if(fd != posix::invalid_descriptor) // if a file descriptor needs to be sent
  {
    header.msg_controllen = CMSG_SPACE(sizeof(int));
    cmsghdr* cmsg = CMSG_FIRSTHDR(&header);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *reinterpret_cast<int*>(CMSG_DATA(cmsg)) = fd;
  }

  flaw(posix::sendmsg(m_socket, &header) == posix::error_response, posix::warning,,false,
       "sendmsg() failure: %s", std::strerror(errno))
  return true;
}

bool ClientSocket::read(posix::fd_t socket, EventData_t event) noexcept
{
  (void)event;
  flaw(m_socket != socket, posix::critical, std::exit(2), false,
       "An 'impossible' situation has occurred.")

  msghdr header = {};
  iovec iov = {};
  char aux_buffer[64] = { 0 };

  header.msg_iov = &iov;
  header.msg_iovlen = 1;
  header.msg_control = aux_buffer;

  iov.iov_base = m_buffer.begin();
  iov.iov_len = m_buffer.capacity();
  header.msg_controllen = CMSG_SPACE(sizeof(int));

  posix::ssize_t byte_count = posix::recvmsg(m_socket, &header, 0);

  flaw(byte_count == posix::error_response, posix::warning,, false,
       "recvmsg() failure: %s", std::strerror(errno))

  flaw(!byte_count, posix::information, disconnect(), false,
       "Socket disconnected.")

  flaw(!m_buffer.resize(byte_count), posix::severe, posix::error(std::errc::not_enough_memory), false,
       "Failed to resize buffer to %li bytes", byte_count)

  posix::fd_t fd = posix::invalid_descriptor;
  if(header.msg_controllen == CMSG_SPACE(sizeof(int)))
  {
    cmsghdr* cmsg = CMSG_FIRSTHDR(&header);
    if(cmsg->cmsg_level == SOL_SOCKET &&
       cmsg->cmsg_type == SCM_RIGHTS &&
       cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
      fd = *reinterpret_cast<int*>(CMSG_DATA(cmsg));
  }
  else
  { flaw(header.msg_flags, posix::warning,, false,
         "error, message flags: 0x%04x", header.msg_flags) }

  Object::enqueue(newMessage, m_socket, m_buffer, fd);
  return true;
}

bool ServerSocket::bind(const char* socket_path, EDomain domain, int socket_backlog) noexcept
{
  flaw(m_connected, posix::warning,,false,
       "Server socket is already bound!")

  flaw(socket_path == nullptr,posix::warning,,false,
       "socket_path is a null value")

  flaw(std::strlen(socket_path) >= sizeof(sockaddr_un::sun_path),posix::warning,,false,
       "socket_path exceeds the maximum path length, %lu bytes", sizeof(sockaddr_un::sun_path))

  m_selfaddr = socket_path;
  m_selfaddr = domain;

  flaw(!posix::bind(m_socket, m_selfaddr, m_selfaddr.size()),posix::warning,,false,
       "Unable to bind to socket to %s: %s", socket_path, std::strerror(errno))
  m_connected = true;

  flaw(!posix::listen(m_socket, socket_backlog),posix::warning,,false,
       "Unable to listen to server socket: %s", std::strerror(errno))
  return true;
}

bool ServerSocket::peerData(posix::fd_t socket, posix::sockaddr_t* addr, proccred_t* creds) const noexcept
{
  auto peer = m_peers.find(socket);
  if(peer == m_peers.end())
    return false;

  if(addr != nullptr)
    *addr = peer->second.addr;
  if(creds != nullptr)
    *creds = peer->second.creds;
  return true;
}


void ServerSocket::acceptPeerRequest(posix::fd_t socket) noexcept
{
  if(m_peers.find(socket) != m_peers.end())
  {
    auto connection = m_connections.emplace(socket, socket).first;
    Object::connect(connection->second.disconnected, this, &ServerSocket::disconnectPeer);
    Object::connect(connection->second.newMessage, newPeerMessage);
    Object::enqueue(connectedPeer, socket);
  }
}

void ServerSocket::rejectPeerRequest(posix::fd_t socket) noexcept
{
  auto peer = m_peers.find(socket);
  if(peer != m_peers.end())
  {
    m_peers.erase(peer);
    posix::close(socket);
  }
}

void ServerSocket::disconnectPeer(posix::fd_t socket) noexcept
{
  m_peers.erase(socket);
  m_connections.erase(socket);
  Object::enqueue(disconnectedPeer, socket);
}


// accepts socket connections and then enqueues newPeerRequest
bool ServerSocket::read(posix::fd_t socket, EventData_t event) noexcept
{
  (void)event;
  flaw(m_socket != socket, posix::critical, std::exit(2), false,
       "An 'impossible' situation has occurred.")
  proccred_t peercred;
  posix::sockaddr_t peeraddr;
  socklen_t addrlen = 0;
  posix::fd_t fd = posix::accept(m_socket, peeraddr, &addrlen); // accept a new socket connection

  flaw(fd == posix::error_response, posix::warning,, false,
       "accept() failure: %s", std::strerror(errno))

  flaw(addrlen >= sizeof(sockaddr_un::sun_path), posix::severe,, false,
       "accept() implementation bug: %s", "address length exceeds availible storage");

  flaw(addrlen != peeraddr.size(), posix::warning,, false,
       "accept() implementation bug: %s", "address length does not match string length");

  flaw(!posix::peercred(fd, peercred), posix::warning,, false,
       "peercred() failure: %s", std::strerror(errno)) // get creditials of connected peer process

  m_peers.emplace(fd, peer_t(fd, peeraddr, peercred));
  Object::enqueue(newPeerRequest, fd, peeraddr, peercred);
  return true;
}

bool ServerSocket::write(posix::fd_t socket, const vfifo& buffer, posix::fd_t fd) const noexcept
{
  auto connection = m_connections.find(socket);
  if(connection != m_connections.end())
    return connection->second.write(buffer, fd);
  return false;
}

