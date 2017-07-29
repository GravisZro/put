#ifndef EVENTBACKEND_H
#define EVENTBACKEND_H

// POSIX
#include <poll.h>
#include <sys/types.h>

// STL
#include <unordered_map>

// PDTK
#include <cxxutils/posix_helpers.h>


enum class EventFlags : uint32_t
{
  Invalid       = 0x00000000,
  Error         = 0x00000001, // FD encountered an error (output only)
  Disconnected  = 0x00000002, // FD has disconnected (output only)
  Readable      = 0x00000004, // FD has content to read
  Writeable     = 0x00000008, // FD is writeable
  EdgeTrigger   = 0x00000010, // FD will be edge-triggered (input only)
  ReadEvent     = 0x00000020, // File/directory was read from
  WriteEvent    = 0x00000040, // File/directory was written to
  AttributeMod  = 0x00000080, // File/directory metadata was modified
  Moved         = 0x00000100, // File/directory was moved
  Deleted       = 0x00000200, // File/directory was deleted
  SubCreated    = 0x00000400, // File/directory was created
  SubMoved      = 0x00000800, // File/directory was created
  SubDeleted    = 0x00001000, // File/directory was created
  FileMod       = 0x000002C0, // Any file modification event
  FileEvent     = 0x00001FE0, // Any file/directory event
  ExecEvent     = 0x00002000, // Process called exec*()
  ExitEvent     = 0x00004000, // Process exited
  ForkEvent     = 0x00008000, // Process forked
  UIDEvent      = 0x00010000, // Process changed its User ID
  GIDEvent      = 0x00020000, // Process changed its Group ID
  SIDEvent      = 0x00040000, // Process changed its Session ID
  ProcEvent     = 0x0007E000, // Any process event

  Any           = 0xFFFFFFFF, // any flag
};
static_assert(sizeof(EventFlags) == sizeof(uint32_t), "EventFlags: bad size");

constexpr uint32_t operator | (EventFlags a, EventFlags b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }

struct EventFlags_t
{
  // FD events
  uint32_t Error        : 1;
  uint32_t Disconnected : 1;
  uint32_t Readable     : 1;
  uint32_t Writeable    : 1;
  uint32_t EdgeTrigger  : 1;
  // FS events
  uint32_t ReadEvent    : 1;
  uint32_t WriteEvent   : 1;
  uint32_t AttributeMod : 1;
  uint32_t Moved        : 1;
  uint32_t Deleted      : 1;
  uint32_t SubCreated   : 1;
  uint32_t SubMoved     : 1;
  uint32_t SubDeleted   : 1;
  // Proc events
  uint32_t ExecEvent    : 1;
  uint32_t ExitEvent    : 1;
  uint32_t ForkEvent    : 1;
  uint32_t UIDEvent     : 1;
  uint32_t GIDEvent     : 1;
  uint32_t SIDEvent     : 1;

  EventFlags_t(uint32_t flags) : EventFlags_t(static_cast<EventFlags>(flags)) { }
  EventFlags_t(EventFlags flags = EventFlags::Invalid) noexcept { *reinterpret_cast<EventFlags*>(this) = flags; }
  operator EventFlags(void) const noexcept { return *reinterpret_cast<const EventFlags*>(this); }

  void unset(EventFlags flags) noexcept { return unset(static_cast<uint32_t>(flags)); }
  void unset(uint32_t flags) noexcept { *reinterpret_cast<uint32_t*>(this) &= *reinterpret_cast<uint32_t*>(this) ^flags; }
  bool isSet(EventFlags flags) const noexcept { return isSet(static_cast<uint32_t>(flags)); }
  bool isSet(uint32_t flags) const noexcept { return *reinterpret_cast<const uint32_t*>(this) & flags; }

  bool operator >= (EventFlags a) const noexcept { return *reinterpret_cast<const uint32_t*>(this) >= static_cast<uint32_t>(a); }
};
static_assert(sizeof(EventFlags_t) == sizeof(EventFlags), "EventFlags_t: bad size");

struct EventData_t
{
  EventFlags_t flags;
  pid_t pid;  // process id
  pid_t tgid; // thread group id
  union
  {
    struct
    {
      uint32_t event_op1;
      uint32_t event_op2;
    };

    struct
    {
      uint32_t exit_code;
      uint32_t exit_signal;
    };

    struct
    {
      pid_t child_pid;
      pid_t child_tgid;
    };

    struct
    {
      uint32_t gid;
      uint32_t egid;
    };

    struct
    {
      uint32_t uid;
      uint32_t euid;
    };
  };

  EventData_t(EventFlags_t _flags = EventFlags::Invalid, uint32_t _pid = 0, uint32_t _tgid = 0, uint32_t op1 = 0, uint32_t op2 = 0)
    : flags(_flags), pid(_pid), tgid(_tgid), event_op1(op1), event_op2(op2) { }
};

struct EventBackend // TODO: convert to namespace
{
  static void init(void) noexcept;
  static void destroy(void) noexcept;

  static posix::fd_t watch(const char* path, EventFlags_t flags = EventFlags::FileMod) noexcept; // add file events to montior
  static posix::fd_t watch(int target, EventFlags_t flags = EventFlags::Readable) noexcept; // add FD or process events to montior

  static bool remove(int target, EventFlags_t flags = EventFlags::Readable) noexcept; // remove from watch queue

  static bool getevents(int timeout = -1) noexcept;

  static std::unordered_multimap<posix::fd_t, EventFlags_t> queue; // watch queue
  static std::unordered_multimap<posix::fd_t, EventData_t> results; // results from getevents()

  static struct platform_dependant* platform;
};

#endif // EVENTBACKEND_H
