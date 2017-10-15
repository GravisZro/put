#ifndef PROCESSEVENT_H
#define PROCESSEVENT_H

// PDTK
#include <object.h>

class ProcessEvent : public Object
{
public:
  enum Flags : uint32_t // Process event flags
  {
    Invalid       = 0x00000000,
    Exec          = 0x00000010, // Process called exec*()
    Exit          = 0x00000020, // Process exited
    Fork          = 0x00000040, // Process forked
    Any           = 0x00000070, // Any process event
  };

  struct Flags_t
  {
    uint32_t       : 4;
    uint32_t Exec  : 1;
    uint32_t Exit  : 1;
    uint32_t Fork  : 1;

    Flags_t(uint32_t flags = 0) noexcept { *reinterpret_cast<uint32_t*>(this) = flags; }
    operator uint32_t& (void) noexcept { return *reinterpret_cast<uint32_t*>(this); }
  };

  ProcessEvent(pid_t _pid, Flags_t _flags) noexcept;
  ~ProcessEvent(void) noexcept;

  pid_t pid(void) const noexcept { return m_pid; }
  Flags_t flags(void) const noexcept { return m_flags; }

  signal<pid_t, Flags_t> activated;
private:
  pid_t m_pid;
  Flags_t m_flags;
  posix::fd_t m_fd;

  struct platform_dependant;
  static struct platform_dependant s_platform;
};

#endif
