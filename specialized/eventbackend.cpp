#include "eventbackend.h"

// PUT
#include <specialized/osdetect.h>
#include <cxxutils/vterm.h>

#if !defined(MAX_EVENTS)
#define MAX_EVENTS 1024
#endif

posix::lockable<std::unordered_multimap<posix::fd_t, EventBackend::callback_info_t>> EventBackend::queue;
std::list<std::pair<posix::fd_t, native_flags_t>> EventBackend::results;

#if defined(__linux__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,5,44) /* Linux 2.5.44+ */

// Linux
#include <sys/epoll.h>

struct EventBackend::platform_dependant // poll notification (epoll)
{
  posix::fd_t fd;
  struct epoll_event output[MAX_EVENTS];

  platform_dependant(void) noexcept
    : fd(posix::invalid_descriptor)
  {
    fd = ::epoll_create(1);
    flaw(fd == posix::invalid_descriptor,
         terminal::critical,
         std::exit(errno),,
         "Unable to create an instance of epoll! %s", std::strerror(errno));
    posix::fcntl(fd, F_SETFD, FD_CLOEXEC); // close on exec*()
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
    native_event.events = uint32_t(flags); // be sure to convert to native events
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

bool EventBackend::poll(milliseconds_t timeout) noexcept
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

#elif defined(__darwin__)     /* Darwin 7+     */ || \
      defined(__DragonFly__)  /* DragonFly BSD */ || \
      (defined(__FreeBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(4,1,0))  /* FreeBSD 4.1+  */ || \
      (defined(__OpenBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,9))    /* OpenBSD 2.9+  */ || \
      (defined(__NetBSD__)  && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,0,0))  /* NetBSD 2+     */

// BSD
#include <sys/time.h>
#include <sys/event.h>

// POSIX
#include <sys/socket.h>

// STL
#include <vector>

struct EventBackend::platform_dependant // poll notification (kqueue)
{
  posix::fd_t kq;
  std::vector<struct kevent> koutput;   // events that were triggered

  static constexpr native_flags_t composite_flag(uint16_t actions, int16_t filters, uint32_t flags) noexcept
    { return native_flags_t(actions) | (native_flags_t(uint16_t(filters)) << 16) | (native_flags_t(flags) << 32); }

  static constexpr short extract_actions(native_flags_t flags) noexcept
    { return flags & 0xFFFF; }

  static constexpr ushort extract_filter(native_flags_t flags) noexcept
    { return (flags >> 16) & 0xFFFF; }

  static constexpr uint32_t extract_flags(native_flags_t flags) noexcept
    { return (short(flags >> 16) & short(EVFILT_TIMER)) == EVFILT_TIMER ? 0 : (flags >> 32); }

  static constexpr uint32_t extract_data(native_flags_t flags) noexcept
    { return (short(flags >> 16) & short(EVFILT_TIMER)) == EVFILT_TIMER ? (flags >> 32) : 0; }

  platform_dependant(void)
  {
    kq = posix::ignore_interruption(::kqueue);
    flaw(kq == posix::error_response,
         terminal::critical,
         std::exit(errno),,
         "Unable to create a new kqueue: %s", std::strerror(errno))
    posix::fcntl(kq, F_SETFD, FD_CLOEXEC); // close on exec*()
    koutput.resize(MAX_EVENTS);
  }

  ~platform_dependant(void)
  {
    posix::close(kq);
  }

  bool add(posix::fd_t fd, native_flags_t flags) noexcept
  {
    struct kevent ev;
    EV_SET(&ev, fd, extract_filter(flags), EV_ADD | extract_actions(flags), extract_flags(flags), extract_data(flags), NULL);
    return ::kevent(kq, &ev, 1, NULL, 0, NULL) == posix::success_response;
  }

  bool remove(posix::fd_t fd) noexcept
  {
    struct kevent ev;
    EV_SET(&ev, fd, 0, EV_DELETE, 0, 0, NULL);
    return ::kevent(kq, &ev, 1, NULL, 0, NULL) == posix::success_response;
  }

} EventBackend::s_platform;

const native_flags_t EventBackend::SimplePollReadFlags = platform_dependant::composite_flag(0, EVFILT_READ, 0);

bool EventBackend::poll(milliseconds_t timeout) noexcept
{
  timespec tout;
  tout.tv_sec = timeout / 1000;
  tout.tv_nsec = (timeout % 1000) * 1000;

  int count = kevent(s_platform.kq, NULL, 0, s_platform.koutput.data(), s_platform.koutput.size(), &tout);
  if(count <= 0)
    return false;

  struct kevent* end = s_platform.koutput.data() + count;
  for(struct kevent* pos = s_platform.koutput.data(); pos != end; ++pos) // iterate through results
    results.emplace_back(std::make_pair(posix::fd_t(pos->ident), platform_dependant::composite_flag(pos->flags, pos->filter, pos->fflags)));
  return true;
}

#elif defined(__unix__)

# if defined(__solaris__) // Solaris / OpenSolaris / OpenIndiana / illumos

#pragma message("The backend code in PUT for Solaris / OpenSolaris / OpenIndiana / illumos is non-functional!  Please submit a patch!")
#pragma message("See: http://docs.oracle.com/cd/E19253-01/816-5168/port-get-3c/index.html")
#  if 0
// Solaris
#include <port.h>

// POSIX
#include <sys/socket.h>

// STL
#include <vector>

struct EventBackend::platform_dependant
{
  posix::fd_t port;
  std::vector<port_event_t> pinput;   // events we want to monitor
  std::vector<port_event_t> poutput;  // events that were triggered

  platform_dependant(void)
  {
    port = ::port_create();
    flaw(port == posix::error_response,
         terminal::critical,
         std::exit(errno),,
         "Unable to create a new poll port: %s", std::strerror(errno))
   pinput .reserve(1024);
   poutput.reserve(1024);
   posix::fcntl(port, F_SETFD, FD_CLOEXEC); // close on exec*()
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

bool EventBackend::poll(milliseconds_t timeout) noexcept
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
#  endif

# elif defined(__QNX__) // QNX
// QNX docs: http://www.qnx.com/developers/docs/7.0.0/index.html#com.qnx.doc.neutrino.devctl/topic/about.html
# pragma message("No event backend code exists in PUT for QNX!  Please submit a patch!")

# elif defined(__hpux__) // HP-UX
// see http://nixdoc.net/man-pages/HP-UX/man7/poll.7.html
// and https://www.freebsd.org/cgi/man.cgi?query=poll&sektion=7&apropos=0&manpath=HP-UX+11.22
// uses /dev/poll
# pragma message("No event backend code exists in PUT for HP-UX!  Please submit a patch!")

#include <sys/devpoll.h>

# elif defined(__aix__) // IBM AIX
// see https://www.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.basetrf1/pollset.htm
// uses pollset_* functions

//#include <sys/poll.h>
//#include <sys/pollset.h>
/*
  pollset_t n;
  n = pollset_create(-1);
  pollset_destroy(n);
*/

# pragma message("No event backend code exists in PUT for IBM AIX!  Please submit a patch!")


# elif defined(__tru64__) // Tru64 (OSF/1)
# pragma message("No event backend code exists in PUT for Tru64!  Please submit a patch!")

# elif defined(__sco_openserver__) // SCO OpenServer
# pragma message("No event backend code exists in PUT for SCO OpenServer!  Please submit a patch!")

# elif defined(__reliant_unix__) // Reliant UNIX
# pragma message("No event backend code exists in PUT for Reliant UNIX!  Please submit a patch!")

# elif defined(BSD)
# pragma message("Unrecognized BSD derivative!")
# endif

# if !defined(__linux__) && /* Linux before epoll*/ \
     !defined(__minix__) /* MINIX */
#pragma message("No platform specific event backend code! Using standard POSIX polling function.")
#endif

#include <poll.h>

// force maximum event count to 1024 per POSIX
#undef MAX_EVENTS
#define MAX_EVENTS 1024

struct EventBackend::platform_dependant // poll notification (epoll)
{
  nfds_t max;
  struct pollfd io[MAX_EVENTS];

  platform_dependant(void) noexcept
  {
    max = 0;
    for(size_t i = 0; i < MAX_EVENTS; ++i)
      io[i].fd = posix::invalid_descriptor;
  }

  bool add(posix::fd_t wd, native_flags_t flags) noexcept
  {
    struct pollfd* pos = io;
    struct pollfd* end = pos + MAX_EVENTS;

    while(pos->fd != wd && pos < end) // search for existing slot
      ++pos;
    if(pos >= end) // if didn't find fd
    {
      pos = io; // reset to beginning of poll list
      while(pos->fd != posix::invalid_descriptor && pos < end) // search fo empty slot
        ++pos;
      if(io + max <= pos) // if adding a new one
        ++max; // increase fd count
    }

    if(pos < end) // if found desired slot
    {
      pos->fd = wd;
      pos->events = short(flags);
    }
    return pos < end;
  }

  bool remove(posix::fd_t wd) noexcept
  {
    struct pollfd* pos = io;
    struct pollfd* end = pos + MAX_EVENTS;

    while(pos->fd != wd && pos < end) // search for existing slot
      ++pos;
    if(pos < end) // if found desired slot
    {
      pos->fd = posix::invalid_descriptor;
      if(io + max >= pos) // if removing the last one
        --max; // decrease fd count
    }
    return pos < end;
  }
} EventBackend::s_platform;

const native_flags_t EventBackend::SimplePollReadFlags = POLLIN;

bool EventBackend::poll(milliseconds_t timeout) noexcept
{
  int rval = posix::ignore_interruption<int, pollfd*, nfds_t, int>(::poll, s_platform.io, s_platform.max, timeout);
  if(rval == posix::error_response)
    return false;

  struct pollfd* pos = s_platform.io;
  struct pollfd* end = pos + s_platform.max;
  for(; pos != end; ++pos) // iterate through results
    results.emplace_back(std::make_pair(pos->fd, native_flags_t(pos->revents))); // save result (in native format)
  return true;
}

#else
# error This platform is not supported.
#endif

bool EventBackend::add(posix::fd_t fd, native_flags_t flags, callback_t function) noexcept
{
  native_flags_t total_flags = flags;
  bool found = false;
  auto iter_pair = queue.equal_range(fd); // find modified entry!
  for(auto& pos = iter_pair.first; pos != iter_pair.second; ++pos)
  {
    total_flags |= pos->second.flags;
    found |= &(pos->second.function) == &function; // save if function was ever found
    if(&(pos->second.function) == &function) // if the FD is in the queue and it's callback function matches
      pos->second.flags |= flags; // simply modify the flags to include the current
  }

  if(!found) // function wasn't found
    queue.emplace(fd, callback_info_t{flags, function}); // make a new entry for this FD

  return s_platform.add(fd, total_flags);
}

bool EventBackend::remove(posix::fd_t fd, native_flags_t flags) noexcept
{
  native_flags_t remaining_flags = 0;
  auto iter_pair = queue.equal_range(fd); // find modified entry!
  auto& pos = iter_pair.first;
  while(pos != iter_pair.second)
  {
    if((pos->second.flags & flags) == pos->second.flags) // if all flags match
      pos = queue.erase(pos);
    else
    {
      if(pos->second.flags & flags) // if only some flags match
      {
        pos->second.flags ^= pos->second.flags & flags; // remove flags
        remaining_flags |= pos->second.flags; // accumulate remaining flags
      }
      ++pos;
    }
  }
  return remaining_flags
      ? s_platform.add(fd, remaining_flags)
      : s_platform.remove(fd);
}

