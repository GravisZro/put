#include "TimerEvent.h"

// POSIX++
#include <climits>
#include <cassert>

// PDTK
#include <specialized/eventbackend.h>

#if defined(__linux__)
#include <linux/version.h>
#elif !defined(KERNEL_VERSION)
#define LINUX_VERSION_CODE 0
#define KERNEL_VERSION(a, b, c) 0
#endif


#if defined(__linux__) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25) /* Linux 2.6.25+ */
#include <sys/timerfd.h>

struct TimerEvent::platform_dependant // timer notification (Linux)
{
  static posix::fd_t add(void) noexcept
  {
    posix::fd_t timer = ::timerfd_create(CLOCK_MONOTONIC, 0);
    if(timer != posix::invalid_descriptor)
      posix::fcntl(timer, F_SETFL, FD_CLOEXEC | O_NONBLOCK);
    return timer;
  }

  static bool set(posix::fd_t fd, microseconds_t delay, microseconds_t repeat_interval)
  {
    struct itimerspec interval_spec;
    interval_spec.it_interval.tv_sec = repeat_interval / 1000000;
    interval_spec.it_interval.tv_nsec = (repeat_interval % 1000000) * 1000;
    interval_spec.it_value.tv_sec = delay / 1000000;
    interval_spec.it_value.tv_nsec = (delay % 1000000) * 1000;
    return ::timerfd_settime(fd, TFD_TIMER_ABSTIME, &interval_spec, NULL) == posix::success_response;
  }

  static bool remove(posix::fd_t fd)
    { return posix::close(fd); }
};

#elif defined(__unix__) || defined(__unix)

# if (defined(__APPLE__) && defined(__MACH__)) /* Darwin 7+     */ || \
      defined(__FreeBSD__)                      /* FreeBSD 4.1+  */ || \
      defined(__DragonFly__)                    /* DragonFly BSD */ || \
      defined(__OpenBSD__)                      /* OpenBSD 2.9+  */ || \
      defined(__NetBSD__)                       /* NetBSD 2+     */
# pragma message("No timer backend code exists in PDTK for BSD derivatives!  Please submit a patch!")

# elif defined(__minix) // MINIX
# pragma message("No timer backend code exists in PDTK for MINIX!  Please submit a patch!")

# elif defined(__QNX__) // QNX
# pragma message("No timer backend code exists in PDTK for QNX!  Please submit a patch!")

# elif defined(__hpux) // HP-UX
# pragma message("No timer backend code exists in PDTK for HP-UX!  Please submit a patch!")

# elif defined(_AIX) // IBM AIX
# pragma message("No timer backend code exists in PDTK for IBM AIX!  Please submit a patch!")

# elif defined(__osf__) || defined(__osf) // Tru64 (OSF/1)
# pragma message("No timer backend code exists in PDTK for Tru64!  Please submit a patch!")

# elif defined(_SCO_DS) // SCO OpenServer
# pragma message("No timer backend code exists in PDTK for SCO OpenServer!  Please submit a patch!")

# elif defined(sinux) // Reliant UNIX
# pragma message("No timer backend code exists in PDTK for Reliant UNIX!  Please submit a patch!")

# elif defined(BSD)
# pragma message("Unrecognized BSD derivative!")

# else
# pragma Unrecognized UNIX variant!
# endif


# if defined(_POSIX_TIMERS)
# pragma message("No platform specific event backend code! Using standard POSIX timer functions.")

#include <cxxutils/vterm.h>

enum {
  Read = 0,
  Write = 1,
};

struct TimerEvent::platform_dependant // timer notification (POSIX)
{
  struct eventinfo_t
  {
    posix::fd_t fd[2]; // two fds for pipe based communication
    timer_t timer;
  };

  std::unordered_map<posix::fd_t , eventinfo_t> events;

  platform_dependant(void) noexcept
  {
    struct sigaction actions;
    actions.sa_sigaction = &handler;
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = SA_SIGINFO | SA_RESTART;

    flaw(::sigaction(SIGALRM, &actions, nullptr) == posix::error_response,
         terminal::critical,
         std::exit(errno),,
         "Unable assign action to a signal: %s", std::strerror(errno))
  }

  static void handler(int, siginfo_t* info, void*) noexcept
  {
    static const uint8_t dummydata = 0; // dummy content
    flaw(posix::write(info->si_value.sival_int, &dummydata, 1) != 1,
         terminal::critical,
         std::exit(errno),, // triggers execution stepper FD
         "Unable to trigger TimerEvent: %s", std::strerror(errno))
  }

  posix::fd_t add(void) noexcept
  {
    eventinfo_t data;
    if(!posix::pipe(data.fd))
      return posix::invalid_descriptor;

    posix::fd_t& readfd = data.fd[Read];
    posix::fcntl(readfd, F_SETFL, FD_CLOEXEC | O_NONBLOCK); // close on exec*() and don't block

    struct sigevent timer_event;
    timer_event.sigev_notify = SIGEV_SIGNAL;
    timer_event.sigev_signo = SIGALRM;
    timer_event.sigev_value.sival_int = data.fd[Write];

    if(::timer_create(CLOCK_MONOTONIC, &timer_event, &data.timer) == posix::error_response)
      return posix::invalid_descriptor;

    if(!events.emplace(readfd, data).second)
      return posix::invalid_descriptor;

    return readfd;
  }

  bool set(posix::fd_t fd, microseconds_t delay, microseconds_t repeat_interval)
  {
    auto iter = events.find(fd);
    if(iter != events.end())
    {
      const eventinfo_t& data = iter->second;
      struct itimerspec interval_spec;
      interval_spec.it_interval.tv_sec = repeat_interval / 1000000;
      interval_spec.it_interval.tv_nsec = (repeat_interval % 1000000) * 1000;
      interval_spec.it_value.tv_sec = delay / 1000000;
      interval_spec.it_value.tv_nsec = (delay % 1000000) * 1000;
      return ::timer_settime(data.timer, 0, &interval_spec, NULL) == posix::success_response;
    }
    return false;
  }

  bool remove(posix::fd_t fd)
  {
    auto iter = events.find(fd);
    if(iter != events.end())
    {
      const eventinfo_t& data = iter->second;
      posix::close(data.fd[Read]);
      posix::close(data.fd[Write]);
      return ::timer_delete(data.timer) == posix::success_response;
    }
    return false;
  }
};
# else
#  error This platform is not supported.
# endif
#else
# error This platform is not supported.
#endif

TimerEvent::TimerEvent(microseconds_t delay, microseconds_t repeat_interval) noexcept
{
  m_fd = s_platform.add();
  EventBackend::add(m_fd, EventBackend::SimplePollReadFlags, // connect communications pipe to a lambda function
                    [this](posix::fd_t fd, native_flags_t) noexcept
  {
    uint64_t discard;
    while(posix::read(fd, &discard, sizeof(discard)) != posix::error_response);
    Object::enqueue(this->expired);
  });

  assert(s_platform.set(m_fd, delay, repeat_interval));
}

TimerEvent::~TimerEvent(void) noexcept
{
  assert(s_platform.remove(m_fd));
}
