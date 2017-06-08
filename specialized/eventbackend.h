#ifndef EVENTBACKEND_H
#define EVENTBACKEND_H

// POSIX
#include <poll.h>
//#include <time.h>
#include <sys/types.h>

// STL
#include <map>
#include <vector>

// PDTK
#include <cxxutils/posix_helpers.h>


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
};
static_assert(sizeof(EventFlags) == sizeof(uint32_t), "EventFlags: bad size");

constexpr uint32_t operator |(EventFlags a, EventFlags b)
  { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
constexpr uint32_t operator &(EventFlags a, EventFlags b)
  { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }

struct EventFlags_t
{
  uint32_t Error        : 1;
  uint32_t Disconnected : 1;
  uint32_t Readable     : 1;
  uint32_t Writeable    : 1;
  uint32_t EdgeTrigger  : 1;
  uint32_t ReadEvent    : 1;
  uint32_t WriteEvent   : 1;
  uint32_t AttributeMod : 1;
  uint32_t Moved        : 1;

  EventFlags_t(EventFlags flags = EventFlags::Invalid) noexcept { *reinterpret_cast<EventFlags*>(this) = flags; }
  operator EventFlags(void) const noexcept { return *reinterpret_cast<const EventFlags*>(this); }
  //EventFlags_t& operator =(const EventFlags_t other) { *reinterpret_cast<EventFlags*>(this) = other; return *this; }
};
static_assert(sizeof(EventFlags_t) == sizeof(EventFlags), "EventFlags_t: bad size");


struct EventBackend // TODO: convert to namespace
{
  static void init(void) noexcept;
  static void destroy(void) noexcept;

  static posix::fd_t watch(const char* path, EventFlags_t events) noexcept; // returns file descriptor upon success or < 0 upon failure
  static bool watch(posix::fd_t fd, EventFlags_t events) noexcept; // sets which events to montior in the watch queue

  static bool remove(posix::fd_t fd) noexcept; // remove from watch queue

  static bool getevents(int timeout = -1) noexcept;

  static std::multimap<posix::fd_t, EventFlags_t> queue; // watch queue
  static std::multimap<posix::fd_t, EventFlags_t> results;

  static struct platform_dependant* platform;
};

#endif // EVENTBACKEND_H
