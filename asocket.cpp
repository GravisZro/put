#include "asocket.h"

// STL
#include <cassert>

// POSIX
#include <unistd.h>

AsyncSocket::AsyncSocket(void)
{
  m_connected = false;

  m_read.socket = posix::socket(EDomain::unix, EType::stream, EProtocol::unspec);
  m_write.socket = dup(m_read.socket);

  m_read .thread = std::thread(&AsyncSocket::async_read , this);
  m_write.thread = std::thread(&AsyncSocket::async_write, this);
}

bool AsyncSocket::is_connected(void)
{
  /*
  int error = 0;
  socklen_t len = sizeof (error);
  int rval = getsockopt (m_read.socket, SOL_SOCKET, SO_ERROR, &error, &len);
  return !rval && !error;
  */
  return m_connected;
}

bool AsyncSocket::bind(const char *socket_path)
{
  if(is_connected())
    return false;

  assert(strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  m_addr = socket_path;
  return m_connected = posix::bind(m_read.socket, m_addr, m_addr.size());
}

bool AsyncSocket::connect(const char *socket_path)
{
  if(is_connected())
    return false;

  assert(strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  m_addr = socket_path;
  m_addr = EDomain::unix;
  return m_connected = posix::connect(m_read.socket, m_addr, m_addr.size());
}

void AsyncSocket::async_read(void)
{
  std::mutex m;
  for(;;)
  {
    std::unique_lock<std::mutex> lk(m);
    m_read.condition.wait(lk, [this] { return is_connected(); } );

    int rval = ::read(m_read.socket, m_read.buffer.data(), m_read.buffer.capacity());
    if(rval != posix::error_response)
      m_read.buffer.resize(rval);
    queue(readFinished, m_read.buffer);
    Application::quit(-1);
  }
}

void AsyncSocket::async_write(void)
{
  std::mutex m;
  for(;;)
  {
    std::unique_lock<std::mutex> lk(m);
    m_write.condition.wait(lk, [this] { return is_connected() && !m_write.buffer.empty(); } );

    ::write(m_write.socket, m_write.buffer.data(), m_write.buffer.size());
    m_write.buffer.resize(0);
    queue(writeFinished);
  }
}

bool AsyncSocket::read(void)
{
  if(!is_connected())
    return false;
  m_read.condition.notify_one();
  return true;
}

bool AsyncSocket::write(const std::vector<char>& buffer)
{
  if(!is_connected())
    return false;
  m_write.buffer.resize(buffer.size());
  memcpy(m_write.buffer.data(), buffer.data(), buffer.size());
  m_write.condition.notify_one();
  return true;
}
