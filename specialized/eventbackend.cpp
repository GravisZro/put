#include "eventbackend.h"

#define MAX_EVENTS 1024

// PDTK
#include <cxxutils/vterm.h>

lockable<std::unordered_multimap<posix::fd_t, EventBackend::callback_info_t>> EventBackend::queue;
std::list<std::pair<posix::fd_t, native_flags_t>> EventBackend::results;

#if defined(__linux__)

// epoll needs kernel 2.5.44

// Linux
#include <sys/epoll.h>

struct EventBackend::platform_dependant // poll notification (epoll)
{
  posix::fd_t fd;
  struct epoll_event output[MAX_EVENTS];

  platform_dependant(void) noexcept
    : fd(posix::invalid_descriptor)
  {
    fd = ::epoll_create(MAX_EVENTS);
    flaw(fd == posix::invalid_descriptor, terminal::critical, std::exit(errno),,
         "Unable to create an instance of epoll! %s", std::strerror(errno))
  }

  ~platform_dependant(void) noexcept
  {
    posix::close(fd);
    fd = posix::invalid_descriptor;
  }

  bool add(posix::fd_t wd, native_flags_t flags) noexcept
  {
    struct epoll_event native_event;
    native_event.data.fd = wd;
    native_event.events = flags; // be sure to convert to native events
    return ::epoll_ctl(fd, EPOLL_CTL_ADD, wd, &native_event) == posix::success_response || // add new event OR
        (errno == EEXIST && ::epoll_ctl(fd, EPOLL_CTL_MOD, wd, &native_event) == posix::success_response); // modify existing event
  }

  bool remove(posix::fd_t wd) noexcept
  {
    struct epoll_event event;
    return ::epoll_ctl(fd, EPOLL_CTL_DEL, wd, &event) == posix::success_response; // try to delete entry
  }
} EventBackend::s_platform;

const native_flags_t EventBackend::SimplePollReadFlags = EPOLLIN;

bool EventBackend::poll(int timeout) noexcept
{
  int count = ::epoll_wait(s_platform.fd, s_platform.output, MAX_EVENTS, timeout); // wait for new results
  results.clear(); // clear old results

  if(count == posix::error_response) // if error/timeout occurred
    return false; //fail

  const epoll_event* end = s_platform.output + count;
  for(epoll_event* pos = s_platform.output; pos != end; ++pos) // iterate through results
    results.emplace_back(std::make_pair(posix::fd_t(pos->data.fd), native_flags_t(pos->events))); // save result (in native format)
  return true;
}

#elif defined(__APPLE__)      /* Darwin 7+     */ || \
      defined(__FreeBSD__)    /* FreeBSD 4.1+  */ || \
      defined(__DragonFly__)  /* DragonFly BSD */ || \
      defined(__OpenBSD__)    /* OpenBSD 2.9+  */ || \
      defined(__NetBSD__)     /* NetBSD 2+     */

// BSD
#include <sys/time.h>
#include <sys/event.h>

// POSIX
#include <sys/socket.h>
#include <unistd.h>

// POSIX++
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cstring>

// STL
#include <vector>
#include <algorithm>
#include <iterator>

// PDTK
#include <cxxutils/vterm.h>
#include <cxxutils/error_helpers.h>

struct EventBackend::platform_dependant // poll notification (epoll)
{
  posix::fd_t kq;
  std::vector<struct kevent> koutput;   // events that were triggered

  static constexpr native_flags_t composite_flag(uint16_t actions, int16_t filters, uint32_t flags) noexcept
    { return native_flags_t(actions) | (native_flags_t(uint16_t(filters)) << 16) | (native_flags_t(flags) << 32); }

  static constexpr short extract_actions(native_flags_t flags) noexcept
    { return flags & 0xFFFF; }

  static constexpr ushort extract_filter(native_flags_t flags) noexcept
    { return (flags >> 16) & 0xFFFF; }

  static constexpr ushort extract_flags(native_flags_t flags) noexcept
    { return flags >> 32; }

  platform_dependant(void)
  {
    kq = posix::ignore_interruption(::kqueue);
    flaw(kq == posix::error_response, terminal::critical, std::exit(errno),,
         "Unable to create a new kqueue: %s", std::strerror(errno))
    koutput.resize(1024);
  }

  ~platform_dependant(void)
  {
    posix::close(kq);
  }

  bool add(posix::fd_t fd, native_flags_t flags) noexcept
  {
    struct kevent ev;
    EV_SET(&ev, fd, extract_filter(flags), EV_ADD | extract_actions(flags), extract_flags(flags), 0, nullptr);
    return kevent(kq, &ev, 1, nullptr, 0, nullptr) == posix::success_response;
  }

  bool remove(posix::fd_t fd) noexcept
  {
    struct kevent ev;
    EV_SET(&ev, fd, 0, EV_DELETE, 0, 0, nullptr);
    return kevent(kq, &ev, 1, nullptr, 0, nullptr) == posix::success_response;
  }

} EventBackend::s_platform;

const native_flags_t EventBackend::SimplePollReadFlags = platform_dependant::composite_flag(0, EVFILT_READ, 0);

bool EventBackend::poll(int timeout) noexcept
{
  timespec tout;
  tout.tv_sec = timeout / 1000;
  tout.tv_nsec = (timeout % 1000) * 1000;

  int count = kevent(s_platform.kq, nullptr, 0, s_platform.koutput.data(), s_platform.koutput.size(), &tout);
  if(count <= 0)
    return false;

  struct kevent* end = s_platform.koutput.data() + count;
  for(struct kevent* pos = s_platform.koutput.data(); pos != end; ++pos) // iterate through results
    results.emplace_back(std::make_pair(posix::fd_t(pos->ident), platform_dependant::composite_flag(pos->flags, pos->filter, pos->fflags)));
  return true;
}



#elif defined(__sun) && defined(__SVR4) // Solaris / OpenSolaris / OpenIndiana / illumos

#pragma message Not implemented, yet!
#pragma message See: http://docs.oracle.com/cd/E19253-01/816-5168/port-get-3c/index.html
#error The backend code in PDTK for Solaris / OpenSolaris / OpenIndiana / illumos is non-functional!  Please submit a patch!

// Solaris
#include <port.h>

// POSIX
#include <sys/socket.h>
#include <unistd.h>

// POSIX++
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cstring>

// STL
#include <vector>

// PDTK
#include <cxxutils/vterm.h>
#include <cxxutils/error_helpers.h>


struct platform_dependant
{
  posix::fd_t port;
  std::vector<port_event_t> pinput;   // events we want to monitor
  std::vector<port_event_t> poutput;  // events that were triggered

  platform_dependant(void)
  {
    port = ::port_create();
    flaw(port == posix::error_response, terminal::critical, std::exit(errno),,
         "Unable to create a new kqueue: %s", std::strerror(errno))
   pinput .reserve(1024);
   poutput.reserve(1024);
  }

  ~platform_dependant(void)
  {
    posix::close(port);
  }
} EventBackend::s_platform;

bool EventBackend::add(posix::fd_t fd, native_flags_t flags, callback_t function) noexcept
{
  port_event_t pev;

  s_platform.pinput.push_back(pev);
  queue.emplace(fd, (callback_info_t){flags, function});
  return true;
}

bool EventBackend::remove(posix::fd_t fd) noexcept
{
  return false;
}

bool EventBackend::poll(int timeout) noexcept
{
  native_flags_t flags;
  uint_t count = 0;
  timespec tout;
  tout.tv_sec = timeout / 1000;
  tout.tv_nsec = (timeout % 1000) * 1000;

  if(::port_getn(s_platform.port,
                 &s_platform.pinput.data(), s_platform.pinput.size(),
                 s_platform.poutput, MAX_EVENTS, &count
                 &tout) == posix::error_response)
    return false;

  port_event_t* end = s_platform.poutput + count;

  for(port_event_t* pos = s_platform.poutput; pos != end; ++pos) // iterate through results
  {
    //flags = from_kevent(*pos);

    results.emplace(posix::fd_t(pos->ident), flags);
  }
  return true;
}

#elif defined(__minix) // MINIX
#error No event backend code exists in PDTK for MINIX!  Please submit a patch!

#elif defined(__QNX__) // QNX
// QNX docs: http://www.qnx.com/developers/docs/7.0.0/index.html#com.qnx.doc.neutrino.devctl/topic/about.html
#error No event backend code exists in PDTK for QNX!  Please submit a patch!

#elif defined(__hpux) // HP-UX
// see http://nixdoc.net/man-pages/HP-UX/man7/poll.7.html
// and https://www.freebsd.org/cgi/man.cgi?query=poll&sektion=7&apropos=0&manpath=HP-UX+11.22
// uses /dev/poll
#error No event backend code exists in PDTK for HP-UX!  Please submit a patch!

#include <sys/devpoll.h>

#elif defined(_AIX) // IBM AIX
// see https://www.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.basetrf1/pollset.htm
// uses pollset_* functions

#include <sys/poll.h>
#include <sys/pollset.h>

  pollset_t n;
  n = pollset_create(-1);
  pollset_destroy(n);

#error No event backend code exists in PDTK for IBM AIX!  Please submit a patch!

#elif defined(BSD)
#error Unrecognized BSD derivative!

#elif defined(__unix__)
#error Unrecognized UNIX variant!

#else
#error This platform is not supported.
#endif

bool EventBackend::add(posix::fd_t fd, native_flags_t flags, callback_t function) noexcept
{
  native_flags_t total_flags = flags;
  bool found = false;
  auto entries = queue.equal_range(fd); // find modified entry!
  for(auto& pos = entries.first; pos != entries.second; ++pos)
  {
    total_flags |= pos->second.flags;
    found |= &(pos->second.function) == &function; // save if function was ever found
    if(&(pos->second.function) == &function) // if the FD is in the queue and it's callback function matches
      pos->second.flags |= flags; // simply modify the flags to include the current
  }

  if(!found) // function wasn't found
    queue.emplace(fd, (callback_info_t){flags, function}); // make a new entry for this FD

  return s_platform.add(fd, total_flags);
}

bool EventBackend::remove(posix::fd_t fd, native_flags_t flags) noexcept
{
  native_flags_t remaining_flags = 0;
  auto entries = queue.equal_range(fd); // find modified entry!
  for(auto& pos = entries.first; pos != entries.second; ++pos)
  {
    if((pos->second.flags & flags) == pos->second.flags) // if all flags match
      queue.erase(pos);
    else if(pos->second.flags & flags) // if only some flags match
    {
      pos->second.flags ^= pos->second.flags & flags; // remove flags
      remaining_flags |= pos->second.flags; // accumulate remaining flags
    }
  }
  return remaining_flags
      ? s_platform.add(fd, remaining_flags)
      : s_platform.remove(fd);
}

