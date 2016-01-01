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
#include "nonposix/getpeercred.h"

class AsyncSocket : public Object
{
public:
  AsyncSocket(void);
  AsyncSocket(AsyncSocket& other);
  AsyncSocket(posix::fd_t socket);
 ~AsyncSocket(void);

  bool bind(const char *socket_path);
  bool listen(int max_connections = SOMAXCONN);

  bool connect(const char *socket_path);

  inline pid_t getpeerpid(void) const { return m_peer.pid; }
  inline gid_t getpeergid(void) const { return m_peer.gid; }
  inline uid_t getpeeruid(void) const { return m_peer.uid; }

  bool read(void);
  inline bool write(posix::fd_t fd) { vqueue b(1); b.resize(1); return write(b, fd); }
  bool write(vqueue& buffer, posix::fd_t fd = posix::invalid_descriptor);

  signal<vqueue&, posix::fd_t> readFinished;  // msesage received
  signal<int>                  writeFinished; // message sent

protected:
  inline bool is_connected(void) const { return m_connected; }
  inline bool is_bound    (void) const { return m_bound    ; }
  void async_read(void);
  void async_write(void);

protected:
  struct async_channel_t
  {
    inline  async_channel_t(void) : buffer(0) { } // empty buffer
    inline ~async_channel_t(void) { posix::close(socket); thread.detach(); }
    posix::fd_t             socket;
    vqueue                  buffer;
    posix::fd_t             fd;
    std::thread             thread;
    std::condition_variable condition;
  };

  std::mutex  m_connection;
  async_channel_t m_read;
  async_channel_t m_write;
  posix::sockaddr_t m_addr;
  bool m_connected;
  bool m_bound;
  proccred_t m_peer;
};

#endif // ASYNCSOCKET_H
