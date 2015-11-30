#ifndef ASYNCSOCKET_H
#define ASYNCSOCKET_H

// STL
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

// project
#include <pdtk.h>

class AsyncSocket : public Object
{
public:
  AsyncSocket(void);

  bool bind(const char *socket_path);
  bool connect(const char *socket_path);

  bool read(void);
  bool write(const std::vector<char>& buffer);

  signal<std::vector<char>> readFinished;
  signal<> writeFinished;

private:
  bool is_connected(void);
  void async_read(void);
  void async_write(void);

private:
  struct async_pkg_t
  {
    inline async_pkg_t(void) { buffer.reserve(0x2000); }
    posix::fd_t             socket;
    std::vector<char>       buffer;
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
