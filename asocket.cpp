#include "asocket.h"

// STL
#include <cassert>

// POSIX
#include <unistd.h>

static_assert(sizeof(uint8_t) == sizeof(char), "size mismatch!");

AsyncSocket::AsyncSocket(void)
  : AsyncSocket(posix::socket(EDomain::unix, EType::stream, EProtocol::unspec))
{
  m_connected = false;
}

AsyncSocket::AsyncSocket(AsyncSocket& other)
  : AsyncSocket(other.m_read.socket)
{
  m_connected = other.m_connected;
}

AsyncSocket::AsyncSocket(posix::fd_t socket)
{
  m_read .socket = dup(socket);
  m_write.socket = dup(socket);

  // socket shutdowns do not behave as expected :(
  //shutdown(m_read .socket, SHUT_WR); // make read only
  //shutdown(m_write.socket, SHUT_RD); // make write only

  m_read .thread = std::thread(&AsyncSocket::async_read , this);
  m_write.thread = std::thread(&AsyncSocket::async_write, this);
}

AsyncSocket::~AsyncSocket(void)
{
  if(m_connected)
  {
    ::close(m_read .socket);
    ::close(m_write.socket);
  }
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

bool AsyncSocket::write(const std::vector<uint8_t>& buffer)
{
  if(!is_connected())
    return false;
  if(buffer.size() > m_write.buffer.capacity())
  {
    errno = SIGXFSZ;
    return false;
  }
  m_write.buffer.resize(buffer.size());
  memcpy(m_write.buffer.data(), buffer.data(), buffer.size());
  m_write.condition.notify_one();
  return true;
}
