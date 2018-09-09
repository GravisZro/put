#ifndef TIMEREVENT_H
#define TIMEREVENT_H

// POSIX++
#include <ctime>

// PDTK
#include <object.h>

typedef uint64_t microseconds_t;

static_assert(sizeof(itimerspec::it_interval.tv_nsec) == sizeof(microseconds_t), "opps");

class TimerEvent : public Object
{
public:
  TimerEvent(microseconds_t delay, microseconds_t repeat_interval = 0) noexcept;
  ~TimerEvent(void) noexcept;

  signal<> expired;
private:
  static void init_once(void) noexcept;
  static void handler(int, siginfo_t* info, void*) noexcept;

  posix::fd_t m_pipeio[2];
  timer_t m_timer;
};

#endif // TIMEREVENT_H
