#ifndef ASYNCSOCKET_H
#define ASYNCSOCKET_H

// STL
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

// PDTK
#include "object.h"
#include "cxxutils/socket_helpers.h"
#include "cxxutils/vqueue.h"

class AsyncSocket : public Object
{
public:
  AsyncSocket(void);
  AsyncSocket(AsyncSocket& other);
  AsyncSocket(posix::fd_t socket);
 ~AsyncSocket(void);

  bool bind(const char *socket_path);
  bool listen(int max_connections = SOMAXCONN, std::vector<const char*> allowed_endpoints = { });

  bool connect(const char *socket_path);

  bool read(void);
  bool write(vqueue& buffer);

  signal<vqueue&> readFinished;
  signal<> writeFinished;

private:
  inline bool is_connected(void) const { return m_connected; }
  inline bool is_bound    (void) const { return m_bound    ; }
  void async_read(void);
  void async_write(void);

private:
  struct async_pkg_t
  {
    inline  async_pkg_t(void) : buffer(0) { } // empty buffer
    inline ~async_pkg_t(void) { posix::close(socket); thread.detach(); }
    posix::fd_t             socket;
    vqueue                  buffer;
    std::thread             thread;
    std::condition_variable condition;
  };

  std::mutex  m_connection;
  async_pkg_t m_read;
  async_pkg_t m_write;
  posix::sockaddr_t m_addr;
  bool m_connected;
  bool m_bound;
};

#endif // ASYNCSOCKET_H
