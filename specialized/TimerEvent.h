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
  posix::fd_t m_fd;
  struct platform_dependant;
  static struct platform_dependant s_platform;
};

#endif // TIMEREVENT_H
