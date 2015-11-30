#include "async_socket.h"

// STL
#include <cassert>

// POSIX
#include <unistd.h>

AsyncSocket::AsyncSocket(void)
{
  m_connection.lock();

  m_read.socket = posix::socket(EDomain::unix, EType::stream, EProtocol::unspec);
  m_write.socket = dup(m_read.socket);

  m_read .thread = std::thread(&AsyncSocket::async_read , this);
  m_write.thread = std::thread(&AsyncSocket::async_write, this);
}

bool AsyncSocket::is_connected(void)
{
  int error = 0;
  socklen_t len = sizeof (error);
  int rval = getsockopt (m_read.socket, SOL_SOCKET, SO_ERROR, &error, &len);
  return !rval && !error;
}

bool AsyncSocket::unlock_connection(bool rvalue)
{
  if(rvalue)
    m_connection.unlock();
  return rvalue;
}

bool AsyncSocket::bind(const char *socket_path)
{
//  if(is_connected())
//    return false;

  assert(strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  m_addr = socket_path;
  return unlock_connection(posix::bind(m_read.socket, m_addr, m_addr.size()));
}

bool AsyncSocket::connect(const char *socket_path)
{
//  if(is_connected())
//    return false;

  assert(strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  m_addr = socket_path;
  m_addr = EDomain::unix;
  return unlock_connection(posix::connect(m_read.socket, m_addr, m_addr.size()));
}

void AsyncSocket::async_read(void)
{
  for(;;)
  {
    m_connection.lock();
    m_read.lock();
    std::cout << "async_read" << std::endl;
    int rval = ::read(m_read.socket, m_read.buffer.data(), m_read.buffer.capacity());
    if(rval != posix::error_response)
      m_read.buffer.resize(rval);
    queue(readFinished, m_read.buffer);
    m_read.unlock();
    m_connection.unlock();
  }
}

void AsyncSocket::async_write(void)
{
  for(;;)
  {
    m_connection.lock();
    m_write.lock();
    std::cout << "async_write" << std::endl;
    ::write(m_write.socket, m_write.buffer.data(), m_write.buffer.size());
    m_write.buffer.resize(0);
    queue(writeFinished);
    m_connection.unlock();
  }
}

bool AsyncSocket::read(void)
{
  if(!is_connected())
    return false;
  m_read.unlock();
  return true;
}

bool AsyncSocket::write(const std::vector<char>& buffer)
{
  if(!is_connected())
    return false;
  m_write.buffer.resize(buffer.size());
  memcpy(m_write.buffer.data(), buffer.data(), buffer.size());
  m_write.unlock();
  return true;
}
