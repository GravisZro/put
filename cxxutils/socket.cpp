#include "socket.h"

using namespace posix;

std::map<fd_t, SocketBase*> SocketBase::s_socket_map;
lockable<fdset_t> SocketBase::s_socket_indexes;


SocketBase::SocketBase(fd_t fd)
  : socket_t(fd)
{
  m_addr = EDomain::unix;
  s_socket_map.emplace(m_socket, this);
  s_socket_indexes.add(m_socket);
}

SocketBase::~SocketBase(void)
{
  s_socket_map.erase(m_socket);
  s_socket_indexes.remove(m_socket);
}

int SocketBase::bytesAvailable(void)
{
  int count = 0;
  if(::ioctl(m_socket, I_NREAD, &count) == posix::error_response) // get the number of bytes that are availible for a read operation
    return posix::error_response;
  return count;
}

inline bool SocketBase::connect(const char *socket_path)
{
  assert(strlen(socket_path) >= sizeof(sockaddr_un::sun_path));
  m_addr = socket_path;
  return posix::connect(m_socket, m_addr, m_addr.size());
}

inline bool SocketBase::bind(const char *socket_path)
{
  assert(strlen(socket_path) >= sizeof(sockaddr_un::sun_path));
  m_addr = socket_path;
  return posix::bind(m_socket, m_addr, m_addr.size());
}

inline bool SocketBase::listen(int max_connections)
  { return posix::listen(m_socket, max_connections); }

void SocketBase::process_socket_queue(void)
{
  int msg_count;
  for(;;)
  {
    std::lock_guard<lockable<fdset_t>> lock(s_socket_indexes); // multithread protection

    // wait for read status change
    msg_count = posix::select(s_socket_indexes);

    for(auto pos = s_socket_map.begin(); msg_count > 0 && pos != s_socket_map.end(); ++pos)
    {
      if(pos->second->bytesAvailable())
      {
        --msg_count;
      }
    }
  }
}
