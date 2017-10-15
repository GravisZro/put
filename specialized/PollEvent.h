#ifndef EVENTS_POLL_H
#define EVENTS_POLL_H

// PDTK
#include <object.h>

class PollEvent : public Object
{
public:
  enum Flags : uint32_t // Pollable file descriptor event flags
  {
    Invalid       = 0x00000000,
    Error         = 0x00000001, // FD encountered an error
    Disconnected  = 0x00000002, // FD has disconnected
    Readable      = 0x00000004, // FD has content to read
    Writeable     = 0x00000008, // FD is writeable
    Any           = 0x0000000F, // Any FD event
  };

  struct Flags_t
  {
    uint32_t Error         : 1;
    uint32_t Disconnected  : 1;
    uint32_t Readable      : 1;
    uint32_t Writeable     : 1;

    Flags_t(uint32_t flags = 0) noexcept { *reinterpret_cast<uint32_t*>(this) = flags; }
    operator uint32_t& (void) noexcept { return *reinterpret_cast<uint32_t*>(this); }
  };

  PollEvent(posix::fd_t _fd, Flags_t _flags) noexcept;
  ~PollEvent(void) noexcept;

  posix::fd_t fd(void) const noexcept { return m_fd; }
  Flags_t flags(void) const noexcept { return m_flags; }

  signal<posix::fd_t, Flags_t> activated;
protected:
  posix::fd_t m_fd;
  Flags_t m_flags;
};

#endif
