#ifndef EVENTS_FILE_H
#define EVENTS_FILE_H

// POSIX
#include <limits.h>

// PDTK
#include <object.h>

class FileEvent : public Object
{
public:
  enum Flags : uint32_t // File flags
  {
    Invalid       = 0x00000000,
    ReadEvent     = 0x00000100, // File was read from
    WriteEvent    = 0x00000200, // File was written to
    AttributeMod  = 0x00000400, // File metadata was modified
    Moved         = 0x00000800, // File was moved
    Deleted       = 0x00001000, // File was deleted
    Modified      = 0x00001E00, // Any file modification event
    Any           = 0x00001F00, // Any file event
  };

  struct Flags_t
  {
    uint32_t               : 8;
    uint32_t ReadEvent     : 1;
    uint32_t WriteEvent    : 1;
    uint32_t AttributeMod  : 1;
    uint32_t Moved         : 1;
    uint32_t Deleted       : 1;

    Flags_t(uint32_t flags = 0) noexcept { *reinterpret_cast<uint32_t*>(this) = flags; }
    operator uint32_t& (void) noexcept { return *reinterpret_cast<uint32_t*>(this); }
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
