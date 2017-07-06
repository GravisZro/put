#ifndef SOCKET_HELPERS_H
#define SOCKET_HELPERS_H

// C++
#include <cstring>

// POSIX
#include <sys/types.h>
#include <sys/un.h>     // for struct sockaddr_un
#include <sys/socket.h> // for socket()
#include <poll.h>       // for poll()
#include <fcntl.h>      // for fcntl()

// PDTK
#include "error_helpers.h"
#include "posix_helpers.h"

enum class EDomain : sa_family_t
{
// POSIX required
  inet      = AF_INET,      //  Internet domain sockets for use with IPv4 addresses.
  inet6     = AF_INET6,     //  Internet domain sockets for use with IPv6 addresses.
  local     = AF_UNIX,      //  UNIX domain sockets.
  unspec    = AF_UNSPEC,    //  Unspecified.

// optional
  ipx       = AF_IPX,       //  IPX - Novell protocols
  netlink   = AF_NETLINK,   //  Kernel user interface device     netlink(7)
  x25       = AF_X25,       //  ITU-T X.25 / ISO-8208 protocol   x25(7)
  ax25      = AF_AX25,      //  Amateur radio AX.25 protocol
  atmpcv    = AF_ATMPVC,    //  Access to raw ATM PVCs
  appletalk = AF_APPLETALK, //  AppleTalk                        ddp(7)
  packet    = AF_PACKET,    //  Low level packet interface       packet(7)
  alg       = AF_ALG,
};

typedef EDomain EProtocol;

enum class EType : int
{
// POSIX required
  stream    = SOCK_STREAM,    //  Provides sequenced, reliable, two-way, connection-based byte streams.  An out-of-band data transmission mechanism may be supported.
  datagram  = SOCK_DGRAM,     //  Supports datagrams (connectionless, unreliable messages of a fixed maximum length).
  seqpacket = SOCK_SEQPACKET, //  Provides a sequenced, reliable, two-way connection-based data transmission path for datagrams of fixed maximum length; a consumer is required to read an entire packet with each input system call.
  raw       = SOCK_RAW,       //  Provides raw network protocol access.

// optional
  rdm       = SOCK_RDM,       //  Provides a reliable datagram layer that does not guarantee ordering.
};


namespace posix
{
  struct sockaddr_t : sockaddr_un
  {
    sockaddr_t(void) noexcept
    {
      operator =(EDomain::unspec);
      std::memset(sun_path, 0, sizeof(sun_path));
    }

    posix::size_t size(void) const noexcept { return sizeof(sun_family) + std::strlen(sun_path); }
    operator struct sockaddr*(void) noexcept { return reinterpret_cast<struct sockaddr*>(this); }
    operator const struct sockaddr*(void) const noexcept { return reinterpret_cast<const struct sockaddr*>(this); }
    operator EDomain(void) const noexcept { return static_cast<EDomain>(sun_family); }
    sockaddr_t& operator = (sa_family_t family) noexcept { sun_family = family; return *this; }
    sockaddr_t& operator = (EDomain family) noexcept { return operator =(static_cast<sa_family_t>(family)); }
    sockaddr_t& operator = (const char* path) noexcept { std::strcpy(sun_path, path); return *this; }
  };

// POSIX wrappers
  static inline fd_t socket(EDomain domain, EType type, EProtocol protocol = EProtocol::unspec, int flags = 0) noexcept
  {
    fd_t fd = ::socket(static_cast<int>(domain),
                       static_cast<int>(type),
                       static_cast<int>(protocol));
    if(fd != error_response)
    {
      ::fcntl(fd, F_SETFD, FD_CLOEXEC);
      if(flags & O_NONBLOCK)
        ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
    }
    return fd;
  }

  static inline fd_t accept(fd_t sockfd, sockaddr* addr = nullptr, socklen_t* addrlen = nullptr, int flags = 0) noexcept
  {
    fd_t fd = ignore_interruption(::accept, sockfd, addr, addrlen);
    if(fd != error_response)
    {
      ::fcntl(fd, F_SETFD, FD_CLOEXEC);
      if(flags & O_NONBLOCK)
        ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
    }
    return fd;
  }

  static inline bool listen(fd_t sockfd, int backlog = SOMAXCONN) noexcept
    { return ::listen(sockfd, backlog) != error_response; }

  static inline bool connect(fd_t sockfd, const sockaddr* addr, socklen_t addrlen) noexcept
    { return ignore_interruption(::connect, sockfd, addr, addrlen) != error_response; }

  static inline bool bind(fd_t sockfd, const sockaddr* addr, socklen_t addrlen) noexcept
    { return ::bind(sockfd, addr, addrlen) != error_response; }

  static inline ssize_t send(fd_t sockfd, const void* buffer, posix::size_t length, int flags = 0) noexcept
    { return ignore_interruption(::send, sockfd, buffer, length, flags); }

  static inline ssize_t recv(fd_t sockfd, void* buffer, posix::size_t length, int flags = 0) noexcept
    { return ignore_interruption(::recv, sockfd, buffer, length, flags); }

  static inline ssize_t sendmsg(fd_t sockfd, const msghdr* msg, int flags = 0) noexcept
    { return ignore_interruption(::sendmsg, sockfd, msg, flags); }

  static inline ssize_t recvmsg(fd_t sockfd, msghdr* msg, int flags = 0) noexcept
    { return ignore_interruption(::recvmsg, sockfd, msg, flags); }

  static inline int poll(pollfd* fds, nfds_t nfds, int timeout = -1) noexcept
    { return ignore_interruption(::poll, fds, nfds, timeout); }
}


#endif // SOCKET_HELPERS_H

