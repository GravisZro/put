#ifndef EVENTS_FILE_H
#define EVENTS_FILE_H

// POSIX
#include <limits.h>

// PDTK
#include <object.h>

class FileEvent : public Object
{
public:
  enum Flags : uint8_t // File flags
  {
    Invalid       = 0x00,
    ReadEvent     = 0x01, // File was read from
    WriteEvent    = 0x02, // File was written to
    AttributeMod  = 0x04, // File metadata was modified
    Moved         = 0x08, // File was moved
    Deleted       = 0x10, // File was deleted
    Modified      = 0x1E, // Any file modification event
    Any           = 0x1F, // Any file event
  };

  struct Flags_t
  {
    uint8_t ReadEvent     : 1;
    uint8_t WriteEvent    : 1;
    uint8_t AttributeMod  : 1;
    uint8_t Moved         : 1;
    uint8_t Deleted       : 1;

    Flags_t(uint8_t flags = 0) noexcept { *reinterpret_cast<uint8_t*>(this) = flags; }
    operator const uint8_t& (void) const noexcept { return *reinterpret_cast<const uint8_t*>(this); }
  };

  FileEvent(const char* _file, Flags_t _flags) noexcept;
  ~FileEvent(void) noexcept;

  const char* file(void) const noexcept { return m_file; }
  Flags_t flags(void) const noexcept { return m_flags; }

  signal<const char*, Flags_t> activated;
private:
  char m_file[PATH_MAX];
  Flags_t m_flags;
  posix::fd_t m_fd;
  struct platform_dependant;
  static struct platform_dependant s_platform;
};

#endif
