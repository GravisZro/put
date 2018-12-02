#include "timerevent.h"

// PUT
#include <specialized/osdetect.h>
#include <specialized/eventbackend.h>

#if defined(FORCE_POSIX_TIMERS)
# pragma message("Forcing use of POSIX timers.")
# define FALLBACK_ON_POSIX_TIMERS

#elif defined(__linux__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,6,25) /* Linux 2.6.25+ */

// Linux
# include <sys/timerfd.h>

// timer notification (Linux)
TimerEvent::TimerEvent(void) noexcept
{
  m_fd = ::timerfd_create(CLOCK_MONOTONIC, 0);
  if(m_fd != posix::invalid_descriptor)
  {
    posix::fcntl(m_fd, F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::donotblock(m_fd); // don't block
  }

  EventBackend::add(m_fd, EventBackend::SimplePollReadFlags, // connect communications pipe to a lambda function
                    [this](posix::fd_t lambda_fd, native_flags_t) noexcept
  {
    uint64_t discard;
    while(posix::read(lambda_fd, &discard, sizeof(discard)) != posix::error_response);
    Object::enqueue(expired);
  });
}

TimerEvent::~TimerEvent(void) noexcept
{
  if(m_fd != posix::invalid_descriptor)
    posix::close(m_fd);
}

bool TimerEvent::start(milliseconds_t delay, bool repeat) noexcept
{
  struct itimerspec interval_spec;
  interval_spec.it_value.tv_sec = delay / 1000;
  interval_spec.it_value.tv_nsec = (delay % 1000) * 1000000;

  if(repeat)
    std::memcpy(&interval_spec.it_interval, &interval_spec.it_value, sizeof(struct timespec));
  else
    std::memset(&interval_spec.it_interval, 0, sizeof(struct timespec));

  return ::timerfd_settime(m_fd, TFD_TIMER_ABSTIME, &interval_spec, NULL) == posix::success_response;
}

bool TimerEvent::stop(void) noexcept
{
  return start(0, true);
}

#elif defined(__darwin__)     /* Darwin 7+     */ || \
      defined(__DragonFly__)  /* DragonFly BSD */ || \
      (defined(__FreeBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(4,1,0))  /* FreeBSD 4.1+  */ || \
      (defined(__OpenBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,9,0))  /* OpenBSD 2.9+  */ || \
      (defined(__NetBSD__)  && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,0,0))  /* NetBSD 2+     */

# include <sys/event.h> // kqueue

static constexpr native_flags_t composite_flag(uint16_t actions, int16_t filters, int32_t flags) noexcept
  { return native_flags_t(actions) | (native_flags_t(uint16_t(filters)) << 16) | (native_flags_t(flags) << 32); }


struct TimerEvent::platform_dependant // timer notification (POSIX)
{
  std::unordered_map<TimerEvent*, posix::fd_t> timers;

  posix::fd_t add(TimerEvent* ptr)
  {
    posix::fd_t fd = posix::dup(STDERR_FILENO);
    if(fd == posix::invalid_descriptor ||
       !timers.emplace(ptr, fd).second)
      return posix::invalid_descriptor;
    return fd;
  }

  void remove(TimerEvent* ptr)
  {
    auto iter = timers.find(ptr);
    if(iter != timers.end())
    {
      posix::close(iter->second);
      timers.erase(iter);
    }
  }
} TimerEvent::s_platform;


TimerEvent::TimerEvent(void) noexcept
  : m_fd(posix::invalid_descriptor)
{
  m_fd = s_platform.add(this);
}

TimerEvent::~TimerEvent(void) noexcept
{
  stop();
  s_platform.remove(this);
}

bool TimerEvent::start(milliseconds_t delay, bool repeat) noexcept
{
  return EventBackend::add(m_fd, composite_flag( repeat ? EV_ONESHOT : 0, EVFILT_TIMER, delay), // connect timer event to lambda function
                    [this](posix::fd_t lambda_fd, native_flags_t lambda_flags) noexcept
                      { Object::enqueue_copy(expired); });
}

bool TimerEvent::stop(void) noexcept
{
  return EventBackend::remove(m_fd, UINT64_MAX); // total removal
}
#else
# pragma message("No platform specific event backend code! Falling back on POSIX timers.")
# define FALLBACK_ON_POSIX_TIMERS
#endif

#if defined(_POSIX_TIMERS) && defined(FALLBACK_ON_POSIX_TIMERS)
// POSIX++
# include <ctime>
# include <climits>
# include <cassert>

// PUT
# include <cxxutils/vterm.h>

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
    posix::fcntl(readfd, F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::donotblock(readfd); // don't block

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

  bool set(posix::fd_t fd, milliseconds_t delay, bool repeat) noexcept
  {
    auto iter = events.find(fd);
    if(iter != events.end())
    {
      const eventinfo_t& data = iter->second;
      struct itimerspec interval_spec;
      interval_spec.it_value.tv_sec = delay / 1000;
      interval_spec.it_value.tv_nsec = (delay % 1000) * 1000000;

      if(repeat)
        std::memcpy(&interval_spec.it_interval, &interval_spec.it_value, sizeof(struct timespec));
      else
        std::memset(&interval_spec.it_interval, 0, sizeof(struct timespec));

      return ::timer_settime(data.timer, 0, &interval_spec, NULL) == posix::success_response;
    }
    return false;
  }

  bool remove(posix::fd_t fd) noexcept
  {
    auto iter = events.find(fd);
    if(iter != events.end())
    {
      eventinfo_t data = iter->second; // copy data
      events.erase(iter);
      posix::close(data.fd[Read]);
      posix::close(data.fd[Write]);
      return ::timer_delete(data.timer) == posix::success_response;
    }
    return false;
  }
} TimerEvent::s_platform;


TimerEvent::TimerEvent(void) noexcept
{
  m_fd = s_platform.add();
  EventBackend::add(m_fd, EventBackend::SimplePollReadFlags, // connect communications pipe to a lambda function
                    [this](posix::fd_t lambda_fd, native_flags_t) noexcept
  {
    uint64_t discard;
    while(posix::read(lambda_fd, &discard, sizeof(discard)) != posix::error_response);
    Object::enqueue(expired);
  });
}

TimerEvent::~TimerEvent(void) noexcept
{
  assert(s_platform.remove(m_fd));
}

bool TimerEvent::start(milliseconds_t delay, bool repeat) noexcept
{
  return s_platform.set(m_fd, delay, repeat);
}

bool TimerEvent::stop(void) noexcept
{
  return start(0, true);
}

#elif !defined(_POSIX_TIMERS) && defined(FALLBACK_ON_POSIX_TIMERS)
# error POSIX timers are not not supported on this platform!
#endif
