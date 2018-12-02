#ifndef EVENTS_POLL_H
#define EVENTS_POLL_H

// PUT
#include <object.h>

class PollEvent : public Object
{
public:
  enum Flags : uint8_t // Pollable file descriptor event flags
  {
    Invalid       = 0x00,
    Error         = 0x01, // FD encountered an error
    Disconnected  = 0x02, // FD has disconnected
    Readable      = 0x04, // FD has content to read
    Writeable     = 0x08, // FD is writeable
    Any           = 0x0F, // Any FD event
  };

  struct Flags_t
  {
    uint8_t Error         : 1;
    uint8_t Disconnected  : 1;
    uint8_t Readable      : 1;
    uint8_t Writeable     : 1;

    Flags_t(uint8_t flags = 0) noexcept { *reinterpret_cast<uint8_t*>(this) = flags; }
    operator const uint8_t& (void) const noexcept { return *reinterpret_cast<const uint8_t*>(this); }
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
