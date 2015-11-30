#ifndef ASYNCSOCKET_H
#define ASYNCSOCKET_H

// STL
#include <vector>
#include <mutex>
#include <thread>

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
  bool unlock_connection(bool rvalue);
  void async_read(void);
  void async_write(void);

private:
  struct async_pkg_t : std::mutex
  {
    inline async_pkg_t(void)
      { buffer.reserve(0x2000); lock(); }
    posix::fd_t       socket;
    std::vector<char> buffer;
    std::thread       thread;
  };

  std::mutex  m_connection;
  async_pkg_t m_read;
  async_pkg_t m_write;
  posix::sockaddr_t m_addr;
};

#endif // ASYNCSOCKET_H
