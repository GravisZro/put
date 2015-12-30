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

  bool getpeereid(uid_t& uid, gid_t& gid)
    { return posix::getpeereid(m_read.socket, uid, gid); }

  bool read(void);
  inline bool write(posix::fd_t fd) { vqueue b(1); b.resize(1); return write(b, fd); }
  bool write(vqueue& buffer, posix::fd_t fd = nullptr);

  signal<vqueue&, posix::fd_t> readFinished;  // msesage received
  signal<int>                  writeFinished; // message sent

protected:
  inline bool is_connected(void) const { return m_connected; }
  inline bool is_bound    (void) const { return m_bound    ; }
  void async_read(void);
  void async_write(void);

protected:
  struct async_pkg_t
  {
    inline  async_pkg_t(void) : buffer(0) { } // empty buffer
    inline ~async_pkg_t(void) { posix::close(socket); thread.detach(); }
    posix::fd_t             socket;
    vqueue                  buffer;
    posix::fd_t             fd;
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
