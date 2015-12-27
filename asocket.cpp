#include "asocket.h"

// STL
#include <cassert>
#include <cstring>
#include <cstdint>
#include <iostream>

// PDTK
#include "cxxutils/error_helpers.h"

#ifndef CMSG_LEN
#define CMSG_LEN(len) ((CMSG_DATA((struct cmsghdr *) NULL) - (unsigned char *) NULL) + len);

#define CMSG_ALIGN(len) (((len) + sizeof(size_t) - 1) & (size_t) ~ (sizeof(size_t) - 1))
#define CMSG_SPACE(len) (CMSG_ALIGN (len) + CMSG_ALIGN (sizeof (struct cmsghdr)))
#define CMSG_LEN(len)   (CMSG_ALIGN (sizeof (struct cmsghdr)) + (len))
#endif

static_assert(sizeof(uint8_t) == sizeof(char), "size mismatch!");

AsyncSocket::AsyncSocket(void)
  : AsyncSocket(posix::socket(EDomain::unix, EType::stream, EProtocol::unspec, 0))
//  : AsyncSocket(posix::socket(EDomain::unix, EType::datagram, EProtocol::unspec, 0))
{
  m_connected = false;
}

AsyncSocket::AsyncSocket(AsyncSocket& other)
  : AsyncSocket(other.m_read.socket)
{
  m_connected = other.m_connected;
  m_bound     = other.m_bound;
}

AsyncSocket::AsyncSocket(posix::fd_t socket)
{
  m_write.socket = dup(socket);
  m_read .socket = dup(socket);
  posix::close(socket);

  // socket shutdowns do not behave as expected :(
  //shutdown(m_read .socket, SHUT_WR); // make read only
  //shutdown(m_write.socket, SHUT_RD); // make write only

  m_read .thread = std::thread(&AsyncSocket::async_read , this);
  m_write.thread = std::thread(&AsyncSocket::async_write, this);
}

AsyncSocket::~AsyncSocket(void)
{
  if(is_bound() || is_connected())
  {
    posix::close(m_read .socket);
    posix::close(m_write.socket);
  }
}

bool AsyncSocket::bind(const char *socket_path)
{
  if(is_bound() || is_connected())
    return false;

  assert(std::strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  m_addr = socket_path;
  return m_bound = posix::bind(m_read.socket, m_addr, m_addr.size());
}

bool AsyncSocket::listen(int max_connections, std::vector<const char*> allowed_endpoints)
{
  if(!is_bound())
    return false;

  bool ok = posix::listen(m_read.socket, max_connections);
  if(ok)
  {
/*
if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
                        perror("accept");
                        exit(1);
                }
*/

  }
  m_connected = ok;
  return ok;
}

bool AsyncSocket::connect(const char *socket_path)
{
  if(is_connected())
    return false;

  assert(std::strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  m_addr = socket_path;
  m_addr = EDomain::unix;
  return m_connected = posix::connect(m_read.socket, m_addr, m_addr.size());
}

void AsyncSocket::async_read(void)
{
  std::mutex m;
  msghdr msg;
  iovec iov;

  msg.msg_name = nullptr;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  for(;;)
  {
    m_read.buffer.allocate(); // allocate 64KB buffer
    std::unique_lock<std::mutex> lk(m);
    m_read.condition.wait(lk, [this] { return is_connected(); } );

    iov.iov_base = m_read.buffer.data();
    iov.iov_len = m_read.buffer.capacity();
    if(m_read.buffer.expand(posix::recvmsg(m_read.socket, &msg, 0)))
    {
      for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg))
      {
      }
      enqueue<vqueue&>(readFinished, m_read.buffer);
    }
    else // error
    {
    }
  }
}

void AsyncSocket::async_write(void)
{
  char non = 0;
  std::mutex m;
  msghdr msg;
  iovec iov;

  msg.msg_name = nullptr;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  for(;;)
  {
    std::unique_lock<std::mutex> lk(m);
    m_write.condition.wait(lk, [this] { return is_connected() && !m_write.buffer.empty(); } );

    switch(m_write.msg_type)
    {
      case messages::data:
        msg.msg_controllen = 0;
        iov.iov_base = m_write.buffer.data();
        iov.iov_len = m_write.buffer.capacity();
        break;

      case messages::credentials:
      case messages::file_descriptor:
        iov.iov_base = &non;
        iov.iov_len = 1;
        msg.msg_control = m_write.buffer.begin();
        msg.msg_controllen = m_write.buffer.capacity();

        cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(posix::fd_t));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = static_cast<int>(m_write.msg_type);
        break;
    }

    int count = posix::sendmsg(m_write.socket, &msg, 0);
    if(count == posix::error_response) // error
    {
      std::cout << std::flush << "error: " << ::strerror(errno) << std::endl << std::flush;
    }
    else
    {
      m_write.buffer.resize(0);
      enqueue(writeFinished, count);
    }
  }
}

bool AsyncSocket::read(void)
{
  if(!is_connected())
    return false;
  m_read.condition.notify_one();
  return true;
}

bool AsyncSocket::write(vqueue& buffer)
{
  if(!is_connected())
    return false;

  m_write.msg_type = messages::data;
  m_write.buffer = buffer; // share buffer memory
  m_write.condition.notify_one();
  return true;
}

bool AsyncSocket::write(posix::fd_t fd)
{
  if(!is_connected() || // ensure connection is active
     !m_write.buffer.allocate(CMSG_SPACE(sizeof(posix::fd_t))) || // allocate memory
     !m_write.buffer.expand  (CMSG_SPACE(0)) || // move end of buffer
     !m_write.buffer.shrink  (CMSG_SPACE(0)) || // move beginning of buffer
     !m_write.buffer.push(fd)) // push the data to the buffer
    return false;
  m_write.msg_type = messages::file_descriptor;
  m_write.condition.notify_one();
  return true;
}
