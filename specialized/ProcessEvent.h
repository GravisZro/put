#ifndef PROCESSEVENT_H
#define PROCESSEVENT_H

// PDTK
#include <object.h>

class ProcessEvent : public Object
{
public:
  enum Flags : uint8_t // Process event flags
  {
    Invalid       = 0x00,
    Exec          = 0x01, // Process called exec*()
    Exit          = 0x02, // Process exited
    Fork          = 0x04, // Process forked
    Any           = 0x07, // Any process event
  };

  struct Flags_t
  {
    uint8_t Exec  : 1;
    uint8_t Exit  : 1;
    uint8_t Fork  : 1;

    Flags_t(uint8_t flags = 0) { *reinterpret_cast<uint8_t*>(this) = flags; }
    operator uint8_t& (void) { return *reinterpret_cast<uint8_t*>(this); }
  };

  ProcessEvent(pid_t _pid, Flags_t _flags);
  ~ProcessEvent(void);

  constexpr pid_t pid(void) const { return m_pid; }
  inline Flags_t flags(void) const { return m_flags; }

  signal<pid_t, Flags_t> activated;
private:
  pid_t m_pid;
  Flags_t m_flags;
  posix::fd_t m_fd;

  struct platform_dependant;
  static struct platform_dependant s_platform;
};

#endif
