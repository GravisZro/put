#ifndef POSIX_TYPES_H
#define POSIX_TYPES_H

// STL
#include <system_error>
#include <cerrno>
#include <cstdint>
#include <cstring>

// POSIX
#include <sys/un.h>     // for struct sockaddr_un
#include <sys/socket.h> // for socket()
#include <sys/select.h> // for select()
#include <sys/types.h>
#include <sys/time.h>   // for struct timeval
#include <fcntl.h>      // for fcntl()


enum class EDomain : sa_family_t
{
// posix required
  inet      = AF_INET,      //  Internet domain sockets for use with IPv4 addresses.
  inet6     = AF_INET6,     //  Internet domain sockets for use with IPv6 addresses.
  unix      = AF_UNIX,      //  UNIX domain sockets.
  unspec    = AF_UNSPEC,    //  Unspecified.

// optional
  local     = AF_LOCAL,     //  alias for unix
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
// posix required
  stream    = SOCK_STREAM,    //  Provides sequenced, reliable, two-way, connection-based byte streams.  An out-of-band data transmission mechanism may be supported.
  datagram  = SOCK_DGRAM,     //  Supports datagrams (connectionless, unreliable messages of a fixed maximum length).
  seqpacket = SOCK_SEQPACKET, //  Provides a sequenced, reliable, two-way connection-based data transmission path for datagrams of fixed maximum length; a consumer is required to read an entire packet with each input system call.
  raw       = SOCK_RAW,       //  Provides raw network protocol access.

// optional
  rdm       = SOCK_RDM,       //  Provides a reliable datagram layer that does not guarantee ordering.
};


constexpr bool operator ==(std::errc err, int err_num)
  { return *reinterpret_cast<int*>(&err) == err_num; }

constexpr bool operator ==(int err_num, std::errc err)
  { return *reinterpret_cast<int*>(&err) == err_num; }


namespace posix
{
  template<typename... Args>
  using function = int(*)(Args...);

  typedef int socket_t;

  static const int error_response = -1;

  template<typename... Args>
  static inline int ignore_interruption(posix::function<Args...> func, Args... args)
  {
    int rval;
    do {
      rval = func(args...);
    } while(rval == error_response && errno == std::errc::interrupted);
    return rval;
  }

  struct sockaddr_t : sockaddr_un
  {
    inline sockaddr_t(void)
    {
      sun_family = static_cast<sa_family_t>(EDomain::unspec);
      std::memset(sun_path, 0, sizeof(sun_path));
    }

    inline int size(void) { return sizeof(sa_family_t) + std::strlen(sun_path); }
    inline operator struct sockaddr*(void) { return reinterpret_cast<struct sockaddr*>(this); }
    inline sockaddr_t& operator = (sa_family_t family) { sun_family = family; return *this; }
    inline sockaddr_t& operator = (const char* path) { std::strcpy(sun_path, path); return *this; }
  };

  struct fdset_t : fd_set
  {
    inline  fdset_t(void) { clear(); }
    inline ~fdset_t(void) { clear(); }

    inline operator fd_set*(void) { return this; }

    inline bool contains(int fd) const { return FD_ISSET (fd, this); }
    inline void clear   (void  ) { FD_ZERO(    this); }
    inline void add     (int fd) { FD_SET (fd, this); }
    inline void remove  (int fd) { FD_CLR (fd, this); }
  };

  static inline socket_t socket(EDomain domain, EType type, EProtocol protocol, int flags = 0)
  {
    socket_t fd = ::socket(static_cast<int>(domain),
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

  static inline socket_t accept(socket_t sockfd, sockaddr* addr, socklen_t* addrlen, int flags = 0)
  {
    socket_t fd = ignore_interruption(::accept, sockfd, addr, addrlen);
    if(fd != error_response)
    {
      ::fcntl(fd, F_SETFD, FD_CLOEXEC);
      if(flags & O_NONBLOCK)
        ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
    }
    return fd;
  }

  static inline bool listen(socket_t sockfd, int backlog)
    { return ::listen(sockfd, backlog) == error_response; }

  static inline bool connect(socket_t sockfd, const sockaddr* addr, socklen_t addrlen)
    { return ignore_interruption(::connect, sockfd, addr, addrlen) == error_response; }

  static inline int select(int max_fd, fd_set* read_set, fd_set* write_set = nullptr, fd_set* except_set = nullptr, timeval* timeout = nullptr)
    { return ignore_interruption(::select, max_fd, read_set, write_set, except_set, timeout); }

  static inline int select(fd_set* read_set, fd_set* write_set = nullptr, fd_set* except_set = nullptr, timeval* timeout = nullptr)
    { return posix::select(FD_SETSIZE, read_set, write_set, except_set, timeout); }
}


#endif // POSIX_TYPES_H

