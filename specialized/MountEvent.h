#ifndef MOUNTEVENT_H
#define MOUNTEVENT_H

// STL
#include <string>

// PDTK
#include <object.h>
#include <specialized/fstable.h>

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
  std::set<struct fsentry_t> m_table;
};

#endif // MOUNTEVENT_H
