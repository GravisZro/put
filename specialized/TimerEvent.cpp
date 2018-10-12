#include "TimerEvent.h"

// POSIX++
#include <climits>

// PDTK
#include <cxxutils/vterm.h>
#include <specialized/eventbackend.h>

#if defined(_POSIX_TIMERS)

void TimerEvent::init_once(void) noexcept
{
  static bool first = true;
  if(first)
  {
    first = false;
    struct sigaction actions;
    actions.sa_sigaction = &handler;
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = SA_SIGINFO | SA_RESTART;

    flaw(::sigaction(SIGALRM, &actions, nullptr) == posix::error_response,
         terminal::critical,
         std::exit(errno),,
         "Unable assign action to a signal: %s", std::strerror(errno))
  }
}

void TimerEvent::handler(int, siginfo_t* info, void*) noexcept
{
  static const uint8_t dummydata = 0; // dummy content
  flaw(posix::write(info->si_value.sival_int, &dummydata, 1) != 1,
       terminal::critical,
       std::exit(errno),, // triggers execution stepper FD
       "Unable to trigger TimerEvent: %s", std::strerror(errno))
}

enum {
  Read = 0,
  Write = 1,
};

TimerEvent::TimerEvent(microseconds_t delay, microseconds_t repeat_interval) noexcept
{
  init_once();
  if(!posix::pipe(m_pipeio))
    terminal::write("posix::pipe()\n");

  posix::fcntl(m_pipeio[Read], F_SETFD, FD_CLOEXEC); // close on exec*()
  posix::donotblock(m_pipeio[Read]); // just in case

  struct sigevent timer_event;
  timer_event.sigev_notify = SIGEV_SIGNAL;
  timer_event.sigev_signo = SIGALRM;
  timer_event.sigev_value.sival_int = m_pipeio[Write];

  if(::timer_create(CLOCK_REALTIME, &timer_event, &m_timer) == posix::error_response)
    terminal::write("timer_create()\n");

  EventBackend::add(m_pipeio[Read], EventBackend::SimplePollReadFlags, // connect communications pipe to a lambda function
                    [this](posix::fd_t fd, native_flags_t) noexcept
  {
    uint64_t discard;
    while(posix::read(fd, &discard, sizeof(discard)) != posix::error_response);
    Object::enqueue(this->expired);
  });

  struct itimerspec interval_spec;
  interval_spec.it_interval.tv_sec = repeat_interval / 1000000;
  interval_spec.it_interval.tv_nsec = (repeat_interval % 1000000) * 1000;
  interval_spec.it_value.tv_sec = delay / 1000000;
  interval_spec.it_value.tv_nsec = (delay % 1000000) * 1000;
  if(::timer_settime(m_timer, 0, &interval_spec, NULL) == posix::error_response)
    terminal::write("timer_settime()\n");
}

TimerEvent::~TimerEvent(void) noexcept
{
  posix::close(m_pipeio[Read]);
  posix::close(m_pipeio[Write]);
  timer_delete(m_timer);
}
#endif
