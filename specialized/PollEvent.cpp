#include "PollEvent.h"

#include <functional>

#if defined(__linux__)
#include <linux/version.h>
#endif

#if defined(__linux__) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,44) /* Linux 2.5.44+ */

// Linux
#include <sys/epoll.h>

// FD flags
static constexpr uint8_t from_native_flags(const native_flags_t flags) noexcept
{
  return
      (flags & EPOLLERR ? PollEvent::Error          : 0) |
      (flags & EPOLLHUP ? PollEvent::Disconnected   : 0) |
      (flags & EPOLLIN  ? PollEvent::Readable       : 0) |
      (flags & EPOLLOUT ? PollEvent::Writeable      : 0);
}

static constexpr native_flags_t to_native_flags(const uint8_t flags) noexcept
{
  return
      (flags & PollEvent::Error        ? native_flags_t(EPOLLERR ) : 0) |
      (flags & PollEvent::Disconnected ? native_flags_t(EPOLLHUP ) : 0) |
      (flags & PollEvent::Readable     ? native_flags_t(EPOLLIN  ) : 0) |
      (flags & PollEvent::Writeable    ? native_flags_t(EPOLLOUT ) : 0);
}

#elif (defined(__APPLE__) && defined(__MACH__)) /* Darwin 7+     */ || \
      defined(__FreeBSD__)                      /* FreeBSD 4.1+  */ || \
      defined(__DragonFly__)                    /* DragonFly BSD */ || \
      defined(__OpenBSD__)                      /* OpenBSD 2.9+  */ || \
      defined(__NetBSD__)                       /* NetBSD 2+     */

#include <sys/event.h> // kqueue

static constexpr native_flags_t composite_flag(uint16_t actions, int16_t filters, uint32_t flags) noexcept
  { return native_flags_t(actions) | (native_flags_t(uint16_t(filters)) << 16) | (native_flags_t(flags) << 32); }

static constexpr bool flag_subset(native_flags_t flags, native_flags_t subset)
  { return (flags & subset) == subset; }

// FD flags
static constexpr uint8_t from_native_flags(const native_flags_t flags) noexcept
{
  return
      (flag_subset(flags, composite_flag(EV_ERROR, 0           , 0)) ? PollEvent::Error        : 0) |
      (flag_subset(flags, composite_flag(EV_EOF  , 0           , 0)) ? PollEvent::Disconnected : 0) |
      (flag_subset(flags, composite_flag(0       , EVFILT_READ , 0)) ? PollEvent::Readable     : 0) |
      (flag_subset(flags, composite_flag(0       , EVFILT_WRITE, 0)) ? PollEvent::Writeable    : 0) ;
}

static constexpr native_flags_t to_native_flags(const uint8_t flags) noexcept
{
  return
      (flags & PollEvent::Error         ? composite_flag(EV_ERROR, 0           , 0) : 0) |
      (flags & PollEvent::Disconnected  ? composite_flag(EV_EOF  , 0           , 0) : 0) |
      (flags & PollEvent::Readable      ? composite_flag(0       , EVFILT_READ , 0) : 0) |
      (flags & PollEvent::Writeable     ? composite_flag(0       , EVFILT_WRITE, 0) : 0) ;
}

#elif defined(__sun) && defined(__SVR4) // Solaris / OpenSolaris / OpenIndiana / illumos
# error No poll event backend code exists in SXinit for Solaris / OpenSolaris / OpenIndiana / illumos!  Please submit a patch!

#elif defined(__minix) // MINIX
# error No poll event backend code exists in PDTK for MINIX!  Please submit a patch!

#elif defined(__QNX__) // QNX
// QNX docs: http://www.qnx.com/developers/docs/7.0.0/index.html#com.qnx.doc.neutrino.devctl/topic/about.html
# error No poll event backend code exists in PDTK for QNX!  Please submit a patch!

#elif defined(__hpux) // HP-UX
// see http://nixdoc.net/man-pages/HP-UX/man7/poll.7.html
// and https://www.freebsd.org/cgi/man.cgi?query=poll&sektion=7&apropos=0&manpath=HP-UX+11.22
// uses /dev/poll
# error No poll event backend code exists in PDTK for HP-UX!  Please submit a patch!

#elif defined(_AIX) // IBM AIX
// see https://www.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.basetrf1/pollset.htm

# error No poll event backend code exists in PDTK for IBM AIX!  Please submit a patch!

#elif defined(BSD)
# error Unrecognized BSD derivative!

#elif defined(__unix__) || defined(__unix)
# error Unrecognized UNIX variant!

#else
# error This platform is not supported.
#endif


PollEvent::PollEvent(posix::fd_t _fd, Flags_t _flags) noexcept
  : m_fd(_fd), m_flags(_flags)
{
  EventBackend::add(m_fd, to_native_flags(m_flags), // connect FD with flags to signal
                    [this](posix::fd_t lambda_fd, native_flags_t lambda_flags) noexcept
                    { Object::enqueue_copy<posix::fd_t, Flags_t>(activated, lambda_fd, from_native_flags(lambda_flags)); });
}

PollEvent::~PollEvent(void) noexcept
{
  EventBackend::remove(m_fd, to_native_flags(m_flags)); // disconnect FD with flags from signal
}
