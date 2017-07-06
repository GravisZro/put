#ifndef ASYNCSOCKET_H
#define ASYNCSOCKET_H

// STL
#include <vector>
#include <list>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// POSIX
#include <poll.h>

// PDTK
#include <object.h>
#include <cxxutils/socket_helpers.h>
#include <cxxutils/vfifo.h>
#include <specialized/peercred.h>
#include <specialized/eventbackend.h>

struct message_t
{
  posix::fd_t socket;
  vfifo       buffer;
  posix::fd_t fd_buffer;
};

class AsyncSocket : public Object,
                    protected EventBackend
{
public:
  AsyncSocket(EDomain   domain    = EDomain::local,
              EType     type      = EType::seqpacket,
              EProtocol protocol  = EProtocol::unspec,
              int       flags     = 0) noexcept;

  AsyncSocket(posix::fd_t socket) noexcept;
 ~AsyncSocket(void) noexcept;

  bool bind(const char *socket_path, int socket_backlog = SOMAXCONN) noexcept;
  bool connect(const char *socket_path) noexcept;

  bool write(message_t msg) noexcept;

  signal<posix::fd_t, message_t>                     readFinished;          // msesage received
  signal<posix::fd_t, posix::ssize_t>                writeFinished;         // message sent
  signal<posix::fd_t, posix::sockaddr_t, proccred_t> connectedToPeer;       // connection is open with peer
  signal<posix::fd_t>                                disconnectedFromPeer;  // connection with peer was severed

protected:
  void async_io    (void) noexcept;
  bool async_spawn (void) noexcept;

private:
  bool has_socket(void) noexcept
    { return m_socket != posix::invalid_descriptor; }
  bool is_bound(void) noexcept
    { return has_socket() && m_bound; }

  bool is_connected(posix::fd_t socket) noexcept
  {
    return has_socket() &&
        queue.find(socket) != queue.end();
  }

private:
  posix::fd_t       m_socket;
  bool              m_bound;    // indicates if in server or client mode
  posix::sockaddr_t m_selfaddr;
  std::thread       m_iothread;

  posix::fd_t m_write_command;
  posix::fd_t m_read_command;

  std::vector<message_t> m_messages;
  message_t m_incomming;
};

class SingleSocket : public AsyncSocket
{
public:
  template<typename... Args>
  SingleSocket(Args... args) noexcept : AsyncSocket(args...), m_fd(posix::invalid_descriptor) { init(); }

  bool write(vfifo buffer, posix::fd_t fd_buffer = posix::invalid_descriptor) noexcept
    { assert(m_fd != posix::invalid_descriptor);
      return AsyncSocket::write({ m_fd, buffer, fd_buffer }); }

  signal<message_t>                     readFinished;          // msesage received
  signal<posix::ssize_t>                writeFinished;         // message sent
  signal<posix::sockaddr_t, proccred_t> connectedToPeer;       // connection is open with peer
  signal<>                              disconnectedFromPeer;  // connection with peer was severed
private:
  void init(void) noexcept
  {
    Object::connect(AsyncSocket::readFinished, this, &SingleSocket::receive);
    Object::connect(AsyncSocket::writeFinished, this, &SingleSocket::sent);
    Object::connect(AsyncSocket::connectedToPeer, this, &SingleSocket::connected);
    Object::connect(AsyncSocket::disconnectedFromPeer, this, &SingleSocket::disconnected);
  }

  void sent(posix::fd_t, posix::ssize_t count) noexcept
    { Object::enqueue(SingleSocket::writeFinished, count); }

  void receive(posix::fd_t, message_t msg) noexcept
    { Object::enqueue(SingleSocket::readFinished, msg); }

  void connected(posix::fd_t fd, posix::sockaddr_t addr, proccred_t cred) noexcept
    { m_fd = fd; Object::enqueue(SingleSocket::connectedToPeer, addr, cred); }

  void disconnected(posix::fd_t) noexcept
    { Object::enqueue(SingleSocket::disconnectedFromPeer); }
private:
  posix::fd_t m_fd;
};

#endif // ASYNCSOCKET_H
