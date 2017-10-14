#ifndef EVENTS_POLL_H
#define EVENTS_POLL_H

// PDTK
#include <object.h>

class PollEvent : public Object
{
public:
  PollEvent(posix::fd_t _fd, Event::Flags _flags);
  ~PollEvent(void);

  constexpr posix::fd_t fd(void) const { return m_fd; }
  inline Event::Flags_t flags(void) const { return m_flags; }

  signal<posix::fd_t, Event::Flags_t> activated;
protected:
  posix::fd_t m_fd;
  Event::Flags_t m_flags;
};

#endif
