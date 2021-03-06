#ifndef MOUNTEVENT_H
#define MOUNTEVENT_H

// STL
#include <string>

// PUT
#include <put/object.h>
#include <put/specialized/fstable.h>

class TimerEvent;

class MountEvent : public Object
{
public:
  MountEvent(void) noexcept;
  ~MountEvent(void) noexcept;

  signal<std::string, std::string> unmounted;
  signal<std::string, std::string> mounted;
private:
  posix::fd_t m_fd;
  TimerEvent* m_timer;
  std::list<struct fsentry_t> m_table;
};

#endif // MOUNTEVENT_H
