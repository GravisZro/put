#ifndef PROCESSEVENT_H
#define PROCESSEVENT_H

// PUT
#include <put/object.h>

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

    Flags_t(uint8_t flags = 0) noexcept { *reinterpret_cast<uint8_t*>(this) = flags; }
    operator const uint8_t& (void) const noexcept { return *reinterpret_cast<const uint8_t*>(this); }
  };

  ProcessEvent(pid_t _pid, Flags_t _flags) noexcept;
  ~ProcessEvent(void) noexcept;

  pid_t pid(void) const noexcept { return m_pid; }
  Flags_t flags(void) const noexcept { return m_flags; }

  signal<pid_t, posix::error_t    > exited; // exit signal with PID and process exit code
  signal<pid_t, posix::Signal::EId> killed; // killed signal with PID and signal number
  signal<pid_t, pid_t             > forked; // fork signal with PID and child PID
  signal<pid_t                    > execed; // exec signal with PID
private:
  pid_t m_pid;
  Flags_t m_flags;
  posix::fd_t m_fd;

  struct platform_dependant;
  static struct platform_dependant s_platform;
};

#endif
