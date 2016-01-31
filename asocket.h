#ifndef ASYNCSOCKET_H
#define ASYNCSOCKET_H

// STL
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

// PDTK
#include <object.h>
#include <cxxutils/socket_helpers.h>
#include <cxxutils/vqueue.h>
#include <specialized/getpeercred.h>

class AsyncSocket : public Object
{
public:
  AsyncSocket(EDomain   domain    = EDomain::unix,
              EType     type      = EType::seqpacket,
              EProtocol protocol  = EProtocol::unspec,
              int       flags     = 0);

  AsyncSocket(posix::fd_t socket);
 ~AsyncSocket(void);

  bool bind(const char *socket_path, int socket_backlog = SOMAXCONN);
  bool connect(const char *socket_path);

  bool read(void);
  inline bool write(posix::fd_t fd) { vqueue b(1); b.resize(1); return write(b, fd); }
  bool write(vqueue& buffer, posix::fd_t fd = posix::invalid_descriptor);

  signal<vqueue&, posix::fd_t> readFinished;  // msesage received
  signal<int>                  writeFinished; // message sent
  signal<posix::sockaddr_t&, proccred_t&> connectedToPeer; // connection is open with peer

protected:
  void async_read (void);
  void async_write(void);
  void async_accept(void);

protected:
  struct async_channel_t
  {
    inline  async_channel_t(void)
      : buffer(0),
        connection(posix::invalid_descriptor),
        fd(posix::invalid_descriptor),
    terminate(false) { }
    inline ~async_channel_t(void) { disconnect(); }

    inline bool is_connected(void) const
      { return connection != posix::invalid_descriptor; }

    inline bool connect(const posix::sockaddr_t& addr)
      { return posix::connect(connection, addr, addr.size()); }

    inline void disconnect(void)
    {
      if(is_connected())
      {
        posix::close(connection);
        connection = posix::invalid_descriptor;
        terminate = true;
        condition.notify_one();
        thread.join();
      }
    }

    vqueue                  buffer;
    posix::fd_t             connection;
    posix::fd_t             fd;
    std::thread             thread;
    std::condition_variable condition;
    bool terminate;
  };

  posix::fd_t       m_socket;
  posix::sockaddr_t m_selfaddr;
  std::thread       m_accept;
  async_channel_t   m_read;
  async_channel_t   m_write;
};

#endif // ASYNCSOCKET_H
