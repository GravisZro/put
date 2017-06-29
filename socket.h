#ifndef SOCKET_H
#define SOCKET_H

// STL
#include <list>

// PDTK
#include <object.h>
#include <cxxutils/socket_helpers.h>
#include <cxxutils/vfifo.h>
#include <specialized/peercred.h>

class GenericSocket : public Object
{
public:
  GenericSocket(posix::fd_t fd) noexcept;
  ~GenericSocket(void) noexcept;

  signal<posix::fd_t> disconnected; // connection with peer was severed
protected:
  virtual bool read(posix::fd_t socket, EventData_t event) noexcept = 0;
  void disconnect(void) noexcept;
  bool m_connected;
  posix::sockaddr_t m_selfaddr;
  posix::fd_t m_socket;
};

class ClientSocket : public GenericSocket
{
public:
  ClientSocket(EDomain   domain   = EDomain::local,
               EType     type     = EType::seqpacket,
               EProtocol protocol = EProtocol::unspec,
               int       flags    = 0) noexcept;

  ClientSocket(posix::fd_t fd) noexcept;

  bool connect(const char *socket_path) noexcept;

  bool write(const vfifo& buffer, posix::fd_t fd = posix::invalid_descriptor) noexcept;

  signal<posix::fd_t, posix::sockaddr_t, proccred_t> connected; // peer is connected
  signal<posix::fd_t, vfifo, posix::fd_t> newMessage; // message received

private:
  bool read(posix::fd_t socket, EventData_t event) noexcept; // buffers incomming data and then enqueues newMessage
  vfifo m_buffer;
};

class ServerSocket : public GenericSocket
{
public:
  ServerSocket(EDomain   domain   = EDomain::local,
               EType     type     = EType::seqpacket,
               EProtocol protocol = EProtocol::unspec,
               int       flags    = 0) noexcept;

  ServerSocket(posix::fd_t fd) noexcept;

  bool bind(const char* socket_path, int socket_backlog) noexcept;

  void acceptPeerRequest(posix::fd_t fd) noexcept;
  void rejectPeerRequest(posix::fd_t fd) const noexcept;

  signal<posix::fd_t, posix::sockaddr_t, proccred_t> newPeerRequest; // peer is requesting a connection
  signal<posix::fd_t> connectedPeer;    // connection with peer was established
  signal<posix::fd_t> disconnectedPeer; // connection with peer was severed

private:
  bool read(posix::fd_t socket, EventData_t event) noexcept; // accepts socket connections and then enqueues newPeerRequest
  std::list<ClientSocket> m_clients;
};

#endif // SOCKET_H
