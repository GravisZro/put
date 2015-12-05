#ifndef POSIX_TYPES_H
#define POSIX_TYPES_H

// STL
#include <system_error>
#include <cerrno>
#include <cstring>

// POSIX
#include <sys/un.h>     // for struct sockaddr_un
#include <sys/socket.h> // for socket()
#include <sys/select.h> // for select()
#include <sys/types.h>
#include <sys/time.h>   // for struct timeval
#include <fcntl.h>      // for fcntl()
#include <signal.h>


enum class ESignal : int
{
  abort               = SIGABRT,    // Process abort signal.
  timer_alarm         = SIGALRM,    // Alarm clock.
  bus_error           = SIGBUS,     // Access to an undefined portion of a memory object.
  child_changed       = SIGCHLD,    // Child process terminated, stopped, or continued.
  continue_execution  = SIGCONT,    // Continue executing, if stopped.
  floating_point      = SIGFPE,     // Erroneous arithmetic operation.
  hangup              = SIGHUP,     // Hangup.
  illegal_instruction = SIGILL,     // Illegal instruction.
  interrupt           = SIGINT,     // Terminal interrupt signal.
  kill                = SIGKILL,    // Kill (cannot be caught or ignored).
  broken_pipe         = SIGPIPE,    // Write on a pipe with no one to read it.
  quit                = SIGQUIT,    // Terminal quit signal.
  segmentation_fault  = SIGSEGV,    // Invalid memory reference.
  stop_execution      = SIGSTOP,    // Stop executing (cannot be caught or ignored).
  terminate           = SIGTERM,    // Termination signal.
  terminal_stop       = SIGTSTP,    // Terminal stop signal.
  tty_input           = SIGTTIN,    // Background process attempting read.
  tty_output          = SIGTTOU,    // Background process attempting write.
  user1               = SIGUSR1,    // User-defined signal 1.
  user2               = SIGUSR2,    // User-defined signal 2.
  polled              = SIGPOLL,    // Pollable event.
  profiling_timer     = SIGPROF,    // Profiling timer expired.
  bad_system_call     = SIGSYS,     // Bad system call.
  debug_breakpoint    = SIGTRAP,    // Trace/breakpoint trap.
  urgent              = SIGURG,     // High bandwidth data is available at a socket.
  virtual_timer       = SIGVTALRM,  // Virtual timer expired.
  cpu_time_exceeded   = SIGXCPU,    // CPU time limit exceeded.
  file_size_exceeded  = SIGXFSZ,    // File size limit exceeded.
};

enum class EDomain : sa_family_t
{
// POSIX required
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
// POSIX required
  stream    = SOCK_STREAM,    //  Provides sequenced, reliable, two-way, connection-based byte streams.  An out-of-band data transmission mechanism may be supported.
  datagram  = SOCK_DGRAM,     //  Supports datagrams (connectionless, unreliable messages of a fixed maximum length).
  seqpacket = SOCK_SEQPACKET, //  Provides a sequenced, reliable, two-way connection-based data transmission path for datagrams of fixed maximum length; a consumer is required to read an entire packet with each input system call.
  raw       = SOCK_RAW,       //  Provides raw network protocol access.

// optional
  rdm       = SOCK_RDM,       //  Provides a reliable datagram layer that does not guarantee ordering.
};


template<typename T>
constexpr bool operator ==(std::errc err, T err_num)
  { return *reinterpret_cast<T*>(&err) == err_num; }

template<typename T>
constexpr bool operator ==(T err_num, std::errc err)
  { return *reinterpret_cast<T*>(&err) == err_num; }

namespace posix
{
  static const int error_response = -1;

  template<typename RType, typename... ArgTypes>
  using function = RType(*)(ArgTypes...);

  typedef int fd_t;

  struct fdset_t : fd_set
  {
    inline ~fdset_t(void) { clear(); }

    inline operator fd_set*(void) { return this; }

    inline bool contains(fd_t fd) const { return FD_ISSET (fd, this); }
    inline void clear   (void   ) { FD_ZERO(    this); }
    inline void add     (fd_t fd) { FD_SET (fd, this); }
    inline void remove  (fd_t fd) { FD_CLR (fd, this); }
  };

  struct socket_t : fdset_t
  {
    socket_t(fd_t s) : m_socket(s) { }
    inline operator fd_t(void) { return m_socket; }
    inline fd_t operator =(fd_t s) { return m_socket = s; }

    fd_t m_socket;
  };

  template<typename RType, typename... ArgTypes>
  static inline RType ignore_interruption(function<RType, ArgTypes...> func, ArgTypes... args)
  {
    RType rval;
    do {
      rval = func(args...);
    } while(rval == error_response && errno == std::errc::interrupted);
    return rval;
  }

  struct sockaddr_t : sockaddr_un
  {
    inline sockaddr_t(void)
    {
      operator =(EDomain::unspec);
      std::memset(sun_path, 0, sizeof(sun_path));
    }

    inline int size(void) { return sizeof(sa_family_t) + std::strlen(sun_path); }
    inline operator struct sockaddr*(void) { return reinterpret_cast<struct sockaddr*>(this); }
    inline sockaddr_t& operator = (sa_family_t family) { sun_family = family; return *this; }
    inline sockaddr_t& operator = (EDomain family) { return operator =(static_cast<sa_family_t>(family)); }
    inline sockaddr_t& operator = (const char* path) { std::strcpy(sun_path, path); return *this; }
  };

  static inline fd_t socket(EDomain domain, EType type, EProtocol protocol, int flags = 0)
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

  static inline fd_t accept(fd_t sockfd, sockaddr* addr, socklen_t* addrlen, int flags = 0)
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

/*
  static inline ssize_t send(socket_t sockfd, const void* buffer, size_t length, int flags = 0)
    { return ignore_interruption(::send, sockfd, buffer, length, flags); }

  static inline ssize_t recv(socket_t sockfd, void* buffer, size_t length, int flags = 0)
    { return ignore_interruption(::recv, sockfd, buffer, length, flags); }
*/

  static inline bool listen(fd_t sockfd, int backlog = SOMAXCONN)
    { return ::listen(sockfd, backlog) != error_response; }

  static inline bool connect(fd_t sockfd, const sockaddr* addr, socklen_t addrlen)
    { return ignore_interruption(::connect, sockfd, addr, addrlen) != error_response; }

  static inline bool bind(fd_t sockfd, const sockaddr* addr, socklen_t addrlen)
    { return ::bind(sockfd, addr, addrlen) != error_response; }

  static inline int select(int max_fd, fd_set* read_set, fd_set* write_set = nullptr, fd_set* except_set = nullptr, timeval* timeout = nullptr)
    { return ignore_interruption(::select, max_fd, read_set, write_set, except_set, timeout); }

  static inline int select(fd_set* read_set, fd_set* write_set = nullptr, fd_set* except_set = nullptr, timeval* timeout = nullptr)
    { return ignore_interruption(::select, FD_SETSIZE, read_set, write_set, except_set, timeout); }
}


#endif // POSIX_TYPES_H

