#ifndef ASYNCSOCKET_H
#define ASYNCSOCKET_H

// STL
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// POSIX
#include <poll.h>

// PDTK
#include <object.h>
#include <cxxutils/socket_helpers.h>
#include <cxxutils/vqueue.h>
#include <specialized/getpeercred.h>

typedef nfds_t index_t;

struct message_t
{
  index_t     index;
  vqueue      buffer;
  posix::fd_t fd_buffer;
};

struct pollfd_t : pollfd
{
  pollfd_t(void) { }
  pollfd_t(int fd, short int events, short int revents)
    : pollfd({fd, events, revents}) { }
};

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

  bool write(message_t msg);

  signal<index_t, message_t>                     readFinished;          // msesage received
  signal<index_t, ssize_t>                       writeFinished;         // message sent
  signal<index_t, posix::sockaddr_t, proccred_t> connectedToPeer;       // connection is open with peer
  signal<index_t>                                disconnectedFromPeer;  // connection with peer was severed

protected:
  void async_io    (void);
  bool async_spawn (void);

private:
  constexpr bool has_socket()
    { return m_socket != posix::invalid_descriptor; }
  constexpr bool is_bound()
    { return has_socket() && m_bound; }

  constexpr bool is_connected(index_t index)
  {
    return has_socket() &&
           m_io.size() > index &&
           m_io.at(index).fd != posix::invalid_descriptor;
  }

  constexpr index_t add(posix::fd_t fd)
  {
    index_t pos = m_io.size();
    if(m_expired.empty())
      m_io.emplace_back(fd, POLLIN, 0);
    else
    {
      pos = m_expired.front();
      m_expired.pop();
      m_io.at(pos) = { fd, POLLIN, 0 };
    }
    return pos;
  }

private:
  posix::fd_t       m_socket;
  bool              m_bound;
  posix::sockaddr_t m_selfaddr;
  std::thread       m_iothread;

  posix::fd_t m_write_command;
  posix::fd_t m_read_command;
  std::vector<pollfd_t> m_io;
  std::queue<index_t> m_expired;

  std::vector<message_t> m_messages;
  message_t m_incomming;
};

class SingleSocket : public AsyncSocket
{
public:
  template<typename... Args>
  inline SingleSocket(Args... args) : AsyncSocket(args...), m_index(0) { init(); }

  inline bool write(vqueue buffer, posix::fd_t fd_buffer = posix::invalid_descriptor)
    { assert(m_index); return AsyncSocket::write({ m_index, buffer, fd_buffer }); }

  signal<message_t>                     readFinished;          // msesage received
  signal<ssize_t>                       writeFinished;         // message sent
  signal<posix::sockaddr_t, proccred_t> connectedToPeer;       // connection is open with peer
  signal<>                              disconnectedFromPeer;  // connection with peer was severed
private:
  inline void init(void)
  {
    Object::connect(AsyncSocket::readFinished, this, &SingleSocket::receive);
    Object::connect(AsyncSocket::writeFinished, this, &SingleSocket::sent);
    Object::connect(AsyncSocket::connectedToPeer, this, &SingleSocket::connected);
    Object::connect(AsyncSocket::disconnectedFromPeer, this, &SingleSocket::disconnected);
  }

  inline void sent(index_t index, ssize_t count)
  {
    assert(m_index == index);
    Object::enqueue(SingleSocket::writeFinished, count);
  }

  inline void receive(index_t index, message_t msg)
  {
    assert(m_index == index);
    Object::enqueue(SingleSocket::readFinished, msg);
  }

  inline void connected(index_t index, posix::sockaddr_t addr, proccred_t cred)
  {
    m_index = index;
    Object::enqueue(SingleSocket::connectedToPeer, addr, cred);
  }

  inline void disconnected(index_t index)
  {
    assert(m_index == index);
    Object::enqueue(SingleSocket::disconnectedFromPeer);
  }

private:
  index_t m_index;
};

#endif // ASYNCSOCKET_H
