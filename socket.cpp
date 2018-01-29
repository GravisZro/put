#include "socket.h"

// STL
#include <memory>

// POSIX
#include <sys/stat.h>

// POSIX++
#include <cstdlib>

// PDTK
#include <cxxutils/error_helpers.h>
#include <cxxutils/vterm.h>


bool use_socket_file(const char* file)
{
  struct stat data;
  if(::stat(file, &data) == posix::success_response)
    return S_ISSOCK(data.st_dev);

  return (errno == EACCES ||
          errno == EOVERFLOW);
}

// GenericSocket

GenericSocket::GenericSocket(EDomain   domain,
                             EType     type,
                             EProtocol protocol,
                             int       flags) noexcept
  : GenericSocket(posix::socket(domain, type, protocol, flags)) { }

GenericSocket::GenericSocket(posix::fd_t socket) noexcept
  : PollEvent(socket, Readable | Disconnected),
    m_connected(false), m_socket(socket)
{
  Object::connect(PollEvent::activated,
                  Object::fslot_t<void, posix::fd_t, Flags_t>(
                    [this](posix::fd_t l_socket, Flags_t l_flags) noexcept
                    {
                      if(l_flags & Readable)
                        read(l_socket, Readable);
                      if(l_flags & Disconnected)
                        Object::enqueue(disconnected, l_socket);
                    }));
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
    Object::disconnect(PollEvent::activated);
    posix::close(m_socket); // connection severed!
    m_socket = posix::invalid_descriptor;
  }

  m_connected = false;
}

// ClientSocket

bool ClientSocket::connect(const char *socket_path) noexcept
{
  flaw(m_connected,
       terminal::warning,,
       false,
       "Client socket is already connected!")

  flaw(std::strlen(socket_path) >= sizeof(sockaddr_un::sun_path),
       terminal::warning,,
       false,
       "socket_path (%lu characters) exceeds the maximum path length (%lu characters)", std::strlen(socket_path), sizeof(sockaddr_un::sun_path))

  posix::sockaddr_t peeraddr;
  proccred_t cred;

  peeraddr = socket_path;
  peeraddr = EDomain::local;
  m_selfaddr = EDomain::unspec;

  flaw(!posix::connect(m_socket, peeraddr, socklen_t(peeraddr.size())),
       terminal::warning,,
       false,
       "connect() to %s \"%s\" failure: %s", (socket_path[0] ? "socket file" : "anonymous socket"), socket_path + (socket_path[0] ? 0 : 1), std::strerror(errno)) // connect to peer process
  m_connected = true;

  flaw(::peercred(m_socket, cred) != posix::success_response,
       terminal::warning,,
       false,
       "peercred() failure: %s", std::strerror(errno)) // get creditials of connected peer process

  Object::enqueue(connected, m_socket, peeraddr, cred);
  return true;
}

bool ClientSocket::write(const vfifo& buffer, posix::fd_t passfd) const noexcept
{
  msghdr header = {};
  iovec iov = {};
  std::unique_ptr<char> aux_buffer(new char[CMSG_SPACE(sizeof(int))]);

  header.msg_iov = &iov;
  header.msg_iovlen = 1;
  header.msg_control = aux_buffer.get();

  iov.iov_base = buffer.begin();
  iov.iov_len = posix::size_t(buffer.size());

  header.msg_controllen = 0;

  if(passfd != posix::invalid_descriptor) // if a file descriptor needs to be sent
  {
    header.msg_controllen = CMSG_SPACE(sizeof(int));
    cmsghdr* cmsg = CMSG_FIRSTHDR(&header);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *reinterpret_cast<int*>(CMSG_DATA(cmsg)) = passfd;
  }

  flaw(posix::sendmsg(m_socket, &header) == posix::error_response,
       terminal::warning,,
       false,
       "sendmsg() failure: %s", std::strerror(errno))
  return true;
}

bool ClientSocket::read(posix::fd_t socket, Flags_t flags) noexcept
{
  (void)flags;
  flaw(m_socket != socket,
       terminal::critical,
       std::exit(int(std::errc::invalid_argument)),
       false,
       "ClientSocket::read() was improperly called: %s", std::strerror(int(std::errc::invalid_argument)))

  msghdr header = {};
  iovec iov = {};
  std::unique_ptr<char> aux_buffer(new char[CMSG_SPACE(sizeof(int))]);

  header.msg_iov = &iov;
  header.msg_iovlen = 1;
  header.msg_control = aux_buffer.get();

  iov.iov_base = m_buffer.begin();
  iov.iov_len = posix::size_t(m_buffer.capacity());
  header.msg_controllen = CMSG_SPACE(sizeof(int));

  posix::ssize_t byte_count = posix::recvmsg(m_socket, &header, 0);

  flaw(byte_count == posix::error_response,
       terminal::warning,,
       false,
       "recvmsg() failure: %s", std::strerror(errno))

  flaw(!byte_count,
       terminal::information,
       disconnect(),
       false,
       "Socket disconnected.")

  flaw(!m_buffer.resize(byte_count),
       terminal::severe,
       posix::error(std::errc::not_enough_memory),
       false,
       "Failed to resize buffer to %li bytes", byte_count)

  posix::fd_t passfd = posix::invalid_descriptor;
  if(header.msg_controllen == CMSG_SPACE(sizeof(int)))
  {
    cmsghdr* cmsg = CMSG_FIRSTHDR(&header);
    if(cmsg->cmsg_level == SOL_SOCKET &&
       cmsg->cmsg_type == SCM_RIGHTS &&
       cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
      passfd = *reinterpret_cast<int*>(CMSG_DATA(cmsg));
  }
  else
  {
    flaw(header.msg_flags,
         terminal::warning,,
         false,
         "error, message flags: 0x%04x", header.msg_flags)
  }

  Object::enqueue(newMessage, m_socket, m_buffer, passfd);
  return true;
}

// ServerSocket

bool ServerSocket::bind(const char* socket_path, EDomain domain, int socket_backlog) noexcept
{
  flaw(m_connected,
       terminal::warning,,
       false,
       "Server socket is already bound!")

  flaw(socket_path == nullptr,
       terminal::warning,,
       false,
       "socket_path is a null value")

  flaw(std::strlen(socket_path) >= sizeof(sockaddr_un::sun_path),
       terminal::warning,,
       false,
      "socket_path (%lu characters) exceeds the maximum path length (%lu characters)", std::strlen(socket_path), sizeof(sockaddr_un::sun_path));

  m_selfaddr = socket_path;
  m_selfaddr = domain;

  flaw(!posix::bind(m_socket, m_selfaddr, socklen_t(m_selfaddr.size())),
       terminal::warning,,
       false,
       "Unable to bind to socket to %s: %s", socket_path, std::strerror(errno))
  m_connected = true;

  flaw(!posix::listen(m_socket, socket_backlog),
       terminal::warning,,
       false,
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
bool ServerSocket::read(posix::fd_t socket, Flags_t flags) noexcept
{
  (void)flags;
  flaw(m_socket != socket,
       terminal::critical,
       std::exit(int(std::errc::invalid_argument)),
       false,
       "ServerSocket::read() was improperly called: %s", std::strerror(int(std::errc::invalid_argument)))
  proccred_t cred;
  posix::sockaddr_t peeraddr;
  socklen_t addrlen = 0;
  posix::fd_t connection = posix::accept(m_socket, peeraddr, &addrlen); // accept a new socket connection

  flaw(connection == posix::error_response,
       terminal::warning,,
       false,
       "accept() failure: %s", std::strerror(errno))

  flaw(addrlen >= sizeof(sockaddr_un::sun_path),
       terminal::severe,,
       false,
       "accept() implementation bug: %s", "address length exceeds availible storage");

  flaw(addrlen != peeraddr.size(),
       terminal::severe,,
       false,
       "accept() implementation bug: %s", "address length does not match string length");

  flaw(::peercred(connection, cred) != posix::success_response,
       terminal::warning,,
       false,
       "peercred() failure: %s", std::strerror(errno)) // get creditials of connected peer process

  m_peers.emplace(connection, peer_t(connection, peeraddr, cred));
  Object::enqueue(newPeerRequest, connection, peeraddr, cred);
  return true;
}

bool ServerSocket::write(posix::fd_t socket, const vfifo& buffer, posix::fd_t passfd) const noexcept
{
  auto connection = m_connections.find(socket);
  if(connection != m_connections.end())
    return connection->second.write(buffer, passfd);
  return false;
}
