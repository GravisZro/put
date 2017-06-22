#ifndef EVENTBACKEND_H
#define EVENTBACKEND_H

// POSIX
#include <poll.h>
//#include <time.h>
#include <sys/types.h>

// STL
#include <unordered_map>
#include <vector>

// PDTK
#include <cxxutils/posix_helpers.h>

#define flaw(condition, ...) \
  if(condition) \
  { \
    dprintf(STDERR_FILENO, "ERROR: "); \
    dprintf(STDERR_FILENO, ##__VA_ARGS__); \
    dprintf(STDERR_FILENO, "\n"); \
    exit(1); \
  }


enum class EventFlags : uint32_t
{
  Invalid       = 0x0000,
  Error         = 0x0001, // FD encountered an error (output only)
  Disconnected  = 0x0002, // FD has disconnected (output only)
  Readable      = 0x0004, // FD has content to read
  Writeable     = 0x0008, // FD is writeable
  EdgeTrigger   = 0x0010, // FD will be edge-triggered (input only)
  ReadEvent     = 0x0020, // File/directory was read from
  WriteEvent    = 0x0040, // File/directory was written to
  AttributeMod  = 0x0080, // File/directory metadata was modified
  Moved         = 0x0100, // File/directory was moved
  ExecEvent     = 0x0200, // Process called exec*()
  ExitEvent     = 0x0400, // Process exited
  ForkEvent     = 0x0800, // Process forked
  UIDEvent      = 0x1000, // Process changed its User ID
  GIDEvent      = 0x2000, // Process changed its Group ID
  SIDEvent      = 0x4000, // Process changed its Session ID
};
static_assert(sizeof(EventFlags) == sizeof(uint32_t), "EventFlags: bad size");

template<typename itype> constexpr uint32_t operator |(EventFlags a, itype b)
  { return static_cast<EventFlags>(a) | static_cast<EventFlags>(b); }
template<typename itype> constexpr uint32_t operator &(EventFlags a, itype b)
  { return static_cast<EventFlags>(a) & static_cast<EventFlags>(b); }
template<typename itype> constexpr uint32_t operator >=(EventFlags a, itype b)
  { return static_cast<EventFlags>(a) >= static_cast<EventFlags>(b); }
template<typename itype> constexpr uint32_t operator |(EventFlags a, EventFlags b)
  { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
template<typename itype> constexpr uint32_t operator &(EventFlags a, EventFlags b)
  { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
template<typename itype> constexpr uint32_t operator >=(EventFlags a, EventFlags b)
  { return static_cast<uint32_t>(a) >= static_cast<uint32_t>(b); }


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
  // Proc events
  uint32_t ExecEvent    : 1;
  uint32_t ExitEvent    : 1;
  uint32_t ForkEvent    : 1;
  uint32_t UIDEvent     : 1;
  uint32_t GIDEvent     : 1;
  uint32_t SIDEvent     : 1;

  EventFlags_t(EventFlags flags = EventFlags::Invalid) noexcept { *reinterpret_cast<EventFlags*>(this) = flags; }
  operator EventFlags(void) const noexcept { return *reinterpret_cast<const EventFlags*>(this); }
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

  static posix::fd_t watch(const char* path, EventFlags_t flags = EventFlags::Readable) noexcept; // add file events to montior
  static posix::fd_t watch(int target, EventFlags_t flags = EventFlags::Readable) noexcept; // add FD or process events to montior

  static bool remove(posix::fd_t fd) noexcept; // remove from watch queue

  static bool getevents(int timeout = -1) noexcept;

  static std::unordered_multimap<posix::fd_t, EventFlags_t> queue; // watch queue
  static std::unordered_multimap<posix::fd_t, EventData_t> results; // results from getevents()

  static struct platform_dependant* platform;
};

#endif // EVENTBACKEND_H
