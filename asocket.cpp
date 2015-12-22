#include "asocket.h"

// STL
#include <cassert>
#include <cstring>
#include <cstdint>


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
  for(;;)
  {
    m_read.buffer.allocate(); // allocate 64KB buffer
    std::unique_lock<std::mutex> lk(m);
    m_read.condition.wait(lk, [this] { return is_connected(); } );

    int rval = ::read(m_read.socket, m_read.buffer.data(), m_read.buffer.capacity());
    if(rval != posix::error_response)
      m_read.buffer.resize(rval);
    enqueue<vqueue&>(readFinished, m_read.buffer);
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
    enqueue(writeFinished);
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

  m_write.buffer = buffer; // share buffer memory
  m_write.condition.notify_one();
  return true;
}
