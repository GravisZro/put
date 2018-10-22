#ifndef TIMEREVENT_H
#define TIMEREVENT_H

// PDTK
#include <object.h>

typedef uint64_t microseconds_t;

class TimerEvent : public Object
{
public:
  TimerEvent(void) noexcept;
  ~TimerEvent(void) noexcept;

  bool start(microseconds_t delay, microseconds_t repeat_interval = 0) noexcept;
  bool stop(void) noexcept;

  signal<> expired;
private:
  posix::fd_t m_fd;
  struct platform_dependant;
  static struct platform_dependant s_platform;
};

#endif // TIMEREVENT_H
