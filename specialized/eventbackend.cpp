#include "eventbackend.h"

#define MAX_EVENTS 1024

#if defined(__linux__)

// epoll needs kernel 2.5.44

// Linux
#include <sys/epoll.h>

// POSIX
#include <fcntl.h>

// POSIX++
#include <cstring> // for strerror()
#include <cstdlib> // for exit

// PDTK
#include <cxxutils/vterm.h>


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
    return ::epoll_ctl(fd, EPOLL_CTL_ADD, wd, &native_event) == posix::success_response;
  }

  bool modify(posix::fd_t wd, native_flags_t flags) noexcept
  {
    struct epoll_event native_event;
    native_event.data.fd = wd;
    native_event.events = flags; // be sure to convert to native events

    return ::epoll_ctl(fd, EPOLL_CTL_MOD, wd, &native_event) == posix::success_response; // try to modify FD first
  }

  bool remove(posix::fd_t wd) noexcept
  {
    struct epoll_event event;
    return ::epoll_ctl(fd, EPOLL_CTL_DEL, wd, &event) == posix::success_response; // try to delete entry
  }
} EventBackend::s_platform;

lockable<std::unordered_multimap<posix::fd_t, EventBackend::callback_info_t>> EventBackend::queue;
std::list<std::pair<posix::fd_t, native_flags_t>> EventBackend::results;

bool EventBackend::add(posix::fd_t fd, native_flags_t flags, callback_t function) noexcept
{
  if(s_platform.add(fd, flags))
    queue.emplace(fd, (callback_info_t){flags, function});
  else if(errno == EEXIST && s_platform.modify(fd, flags)) // if entry exist and modification worked
  {
    bool found = false;
    auto entries = queue.equal_range(fd); // find modified entry!
    for(auto& pos = entries.first; pos != entries.second; ++pos)
    {
      found |= &(pos->second.function) == &function; // save if function was ever found
      if(&(pos->second.function) == &function) // if the FD is in the queue and it's callback function matches
        pos->second.flags |= flags; // simply modify the flags to include the current
    }

    if(!found) // function wasn't found
      queue.emplace(fd, (callback_info_t){flags, function}); // make a new entry for this FD
  }
  else // unable to add an entry or modify the entry for this FD!
    return false; // fail

  return true;
}

bool EventBackend::remove(posix::fd_t fd, native_flags_t flags) noexcept
{
  auto entries = queue.equal_range(fd); // find modified entry!
  for(auto& pos = entries.first; pos != entries.second; ++pos)
  {
    if(pos->second.flags == flags)
      queue.erase(pos);
    else if(pos->second.flags & flags)
    {
      pos->second.flags ^= pos->second.flags & flags;
      if(pos->second.flags) // if some flags are still set
        s_platform.modify(fd, pos->second.flags);
      else // if no flags are set then remove it completely
        s_platform.remove(fd);
    }
  }
  return true;
}

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

namespace EventBackend
{
  static posix::fd_t s_pipeio[2] = { posix::invalid_descriptor }; //  execution stepper pipe
  static vfunc stepper_function = nullptr;
  enum {
    Read = 0,
    Write = 1,
  };


  void readstep(posix::fd_t fd, native_flags_t) noexcept
  {
    uint64_t discard;
    while(posix::read(fd, &discard, sizeof(discard)) != posix::error_response);
    stepper_function();
  }

  bool setstepper(vfunc function) noexcept
  {
    stepper_function = function;
    if(s_pipeio[Read] == posix::invalid_descriptor) // if execution stepper pipe  hasn't been initialized yet
    {
      flaw(::pipe(s_pipeio) == posix::error_response, terminal::critical, std::exit(errno), false,
           "Unable to create pipe for execution stepper: %s", std::strerror(errno))
      ::fcntl(s_pipeio[Read], F_SETFD, FD_CLOEXEC);
      ::fcntl(s_pipeio[Read], F_SETFL, O_NONBLOCK);
      return EventBackend::add(s_pipeio[Read], EPOLLIN, readstep); // watch for when execution stepper pipe has been triggered
    }
    return stepper_function != nullptr;
  }

  void step(void) noexcept
  {
    static const uint8_t dummydata = 0; // dummy content
    flaw(posix::write(s_pipeio[Write], &dummydata, 1) != 1, terminal::critical, /*std::exit(errno)*/,, // triggers execution stepper FD
         "Unable to trigger Object signal queue processor: %s", std::strerror(errno))
  }
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

// PDTK
#include <cxxutils/vterm.h>
#include <cxxutils/error_helpers.h>


lockable<std::unordered_multimap<posix::fd_t, EventBackend::callback_info_t>> EventBackend::queue;
std::list<std::pair<posix::fd_t, native_flags_t>> EventBackend::results;

struct EventBackend::platform_dependant // poll notification (epoll)
{
  posix::fd_t kq;
  std::vector<struct kevent> kinput;    // events we want to monitor
  std::vector<struct kevent> koutput;   // events that were triggered

  platform_dependant(void)
  {
    kq = posix::ignore_interruption(::kqueue);
    flaw(kq == posix::error_response, terminal::critical, std::exit(errno),,
         "Unable to create a new kqueue: %s", std::strerror(errno))
    kinput .reserve(1024);
    koutput.reserve(1024);
  }

  ~platform_dependant(void)
  {
    posix::close(kq);
  }
} EventBackend::s_platform;


static constexpr short extract_filter(native_flags_t flags) noexcept
  { return flags & 0xFFFF; }

static constexpr ushort extract_flags(native_flags_t flags) noexcept
  { return flags >> 16; }


bool EventBackend::add(posix::fd_t fd, native_flags_t flags, callback_t function) noexcept
{
  struct kevent ev;
  EV_SET(&ev, fd, extract_filter(flags), EV_ADD, extract_flags(flags), 0, nullptr);
  s_platform.kinput.push_back(ev);
  s_platform.koutput.resize(s_platform.kinput.size());
  queue.emplace(fd, (callback_info_t){flags, function});
  return true;
}

bool EventBackend::remove(posix::fd_t fd, native_flags_t flags) noexcept
{
  return false;
}

bool EventBackend::poll(int timeout) noexcept
{
  uint32_t data;
  timespec tout;
  tout.tv_sec = timeout / 1000;
  tout.tv_nsec = (timeout % 1000) * 1000;

  int count = 0;
  count = kevent(s_platform.kq,
             s_platform.kinput.data(), s_platform.kinput.size(),
             s_platform.koutput.data(), s_platform.koutput.size(),
             &tout);

  if(count <= 0)
    return false;

  struct kevent* end = s_platform.koutput.data() + count;

  for(struct kevent* pos = s_platform.koutput.data(); pos != end; ++pos) // iterate through results
    results.emplace_back(std::make_pair(posix::fd_t(pos->ident), native_flags_t((reinterpret_cast<uint16_t>(pos->filter) << 16) | pos->flags)));
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

bool EventBackend::remove(posix::fd_t fd, native_flags_t flags) noexcept
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
