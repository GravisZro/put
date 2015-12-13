#ifndef ASYNCSOCKET_H
#define ASYNCSOCKET_H

// STL
#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

// POSIX
#include <unistd.h>

// project
#include "object.h"
#include "cxxutils/socket_helpers.h"

class AsyncSocket : public Object
{
public:
  AsyncSocket(void);
  AsyncSocket(AsyncSocket& other);
  AsyncSocket(posix::fd_t socket);
 ~AsyncSocket(void);

  bool bind(const char *socket_path);
  bool connect(const char *socket_path);

  bool read(void);
  bool write(const std::vector<uint8_t>& buffer);

  signal<std::vector<uint8_t>> readFinished;
  signal<> writeFinished;

private:
  bool is_connected(void);
  void async_read(void);
  void async_write(void);

private:
  struct async_pkg_t
  {
    inline async_pkg_t(void) { buffer.reserve(0x2000); }
    inline ~async_pkg_t(void) { ::close(socket); thread.detach(); }
    posix::fd_t             socket;
    std::vector<uint8_t>       buffer;
    std::thread             thread;
    std::condition_variable condition;
  };

  std::mutex  m_connection;
  async_pkg_t m_read;
  async_pkg_t m_write;
  posix::sockaddr_t m_addr;
  bool m_connected;
};

#endif // ASYNCSOCKET_H
