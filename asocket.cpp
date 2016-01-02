#include "asocket.h"

// STL
#include <cassert>
#include <cstring>
#include <cstdint>
#include <iostream>

// PDTK
#include "cxxutils/error_helpers.h"
#include "cxxutils/streamcolors.h"
#include "nonposix/getpeercred.h"

#ifndef CMSG_LEN
#define CMSG_ALIGN(len) (((len) + sizeof(size_t) - 1) & (size_t) ~ (sizeof(size_t) - 1))
#define CMSG_SPACE(len) (CMSG_ALIGN (len) + CMSG_ALIGN (sizeof (struct cmsghdr)))
#define CMSG_LEN(len)   (CMSG_ALIGN (sizeof (struct cmsghdr)) + (len))
#endif

static_assert(sizeof(uint8_t) == sizeof(char), "size mismatch!");

namespace posix
{
  inline bool getpeercred(fd_t sockfd, proccred_t& cred)
    { return ::getpeercred(sockfd, cred) == posix::success; }
}

AsyncSocket::AsyncSocket(void)
{
  const int enable = 1;
  m_socket = posix::socket(EDomain::unix, EType::stream, EProtocol::unspec, 0);
  assert(setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == posix::success);
}

AsyncSocket::AsyncSocket(posix::fd_t socket)
  : m_socket(socket)
{
}

AsyncSocket::~AsyncSocket(void)
{
  if(m_read.is_connected())
    m_read.disconnect();
  if(m_write.is_connected())
    m_write.disconnect();
  if(m_socket != posix::invalid_descriptor)
    posix::close(m_socket);
  m_socket = posix::invalid_descriptor;
  if(m_selfaddr != EDomain::unspec)
    ::unlink(m_selfaddr.sun_path);
  m_accept.detach();

}

bool AsyncSocket::bind(const char *socket_path, int socket_backlog)
{
  if(m_read.is_connected())
    return false;

  assert(std::strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  m_selfaddr = socket_path;
  m_selfaddr = EDomain::unix;
  bool ok = posix::bind(m_socket, m_selfaddr, m_selfaddr.size());
  ok = ok && posix::listen(m_socket, socket_backlog);
  if(ok)
    m_accept = std::thread(&AsyncSocket::async_accept, this);
  return ok;
}

void AsyncSocket::async_accept(void)
{
  posix::sockaddr_t m_peeraddr;
  proccred_t m_peercred;

  socklen_t addrlen;
  m_read.connection = posix::accept(m_socket, m_peeraddr, &addrlen);
  m_write.connection = ::dup(m_read.connection);
  if(m_read.is_connected() &&
     posix::getpeercred(m_read.connection, m_peercred))
  {
    enqueue(connectedToPeer, m_peeraddr, m_peercred);
    m_read .thread = std::thread(&AsyncSocket::async_read , this);
    m_write.thread = std::thread(&AsyncSocket::async_write, this);
  }
}

bool AsyncSocket::connect(const char *socket_path)
{
  if(m_read.is_connected())
    return false;

  assert(std::strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  proccred_t m_peercred;
  posix::sockaddr_t m_peeraddr;
  m_peeraddr = socket_path;
  m_peeraddr = EDomain::unix;
  m_selfaddr = EDomain::unspec;

  m_read.connection = m_socket;
  bool ok = m_read.connect(m_peeraddr);
  ok = ok && posix::getpeercred(m_read.connection, m_peercred);
  if(ok)
  {
    m_write.connection = ::dup(m_read.connection);
    enqueue(connectedToPeer, m_peeraddr, m_peercred);
    m_read .thread = std::thread(&AsyncSocket::async_read , this);
    m_write.thread = std::thread(&AsyncSocket::async_write, this);
  }
  else
    m_read.connection = posix::invalid_descriptor;
  return ok;
}

void AsyncSocket::async_read(void)
{
  std::mutex m;
  msghdr msg = {};
  iovec iov = {};

  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  for(;;)
  {
    m_read.buffer.allocate(); // allocate 64KB buffer
    std::unique_lock<std::mutex> lk(m);
    m_read.condition.wait(lk, [this] { return m_read.is_connected(); } );

    iov.iov_base = m_read.buffer.data();
    iov.iov_len = m_read.buffer.capacity();
    if(m_read.buffer.expand(posix::recvmsg(m_read.connection, &msg, 0)))
    {
      if(msg.msg_controllen == CMSG_SPACE(sizeof(int)))
      {
        cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        if(cmsg->cmsg_level == SOL_SOCKET &&
           cmsg->cmsg_type == SCM_RIGHTS &&
           cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
         m_read.fd = *reinterpret_cast<int*>(CMSG_DATA(cmsg));
      }
      else
        m_read.fd = posix::invalid_descriptor;
      enqueue<vqueue&, posix::fd_t>(readFinished, m_read.buffer, m_read.fd);
    }
    else // error
      std::cout << std::flush << std::endl << std::red << "error: " << ::strerror(errno) << std::none << std::endl << std::flush;
  }
}

void AsyncSocket::async_write(void)
{
  std::mutex m;
  msghdr msg = {};
  iovec iov = {};
  char aux_buffer[CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(ucred))] = { 0 };

  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = aux_buffer;

  for(;;)
  {
    std::unique_lock<std::mutex> lk(m);
    m_write.condition.wait(lk, [this] { return m_write.is_connected() && !m_write.buffer.empty(); } );

    iov.iov_base = m_write.buffer.begin();
    iov.iov_len = m_write.buffer.size();

    msg.msg_controllen = 0;

    if(m_write.fd != posix::invalid_descriptor)
    {
      msg.msg_controllen = CMSG_SPACE(sizeof(int));
      cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
      cmsg->cmsg_len = CMSG_LEN(sizeof(int));
      *reinterpret_cast<int*>(CMSG_DATA(cmsg)) = m_write.fd;
    }

    int count = posix::sendmsg(m_write.connection, &msg, 0);
    if(count == posix::error_response) // error
      std::cout << std::flush << std::endl << std::red << "error: " << ::strerror(errno) << std::none << std::endl << std::flush;
    else
      enqueue(writeFinished, count);
    m_write.buffer.resize(0);
  }
}

bool AsyncSocket::read(void)
{
  if(!m_read.is_connected())
    return false;
  m_read.condition.notify_one();
  return true;
}

bool AsyncSocket::write(vqueue& buffer, posix::fd_t fd)
{
  if(!m_read.is_connected())
    return false;

  m_write.fd = fd;
  m_write.buffer = buffer; // share buffer memory
  m_write.condition.notify_one();
  return true;
}
