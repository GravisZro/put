#ifndef EVENTBACKEND_H
#define EVENTBACKEND_H

// STL
#include <mutex>
#include <functional>
#include <unordered_map>
#include <list>

// PDTK
#include <cxxutils/posix_helpers.h>

template<typename T>
struct lockable : T, std::mutex
  { template<typename... ArgTypes> constexpr lockable(ArgTypes... args) noexcept : T(args...) { } };

namespace Event
{
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
    operator uint8_t& (void) noexcept { return *reinterpret_cast<uint8_t*>(this); }
  };
}

namespace EventBackend
{
  using callback_t = std::function<void(posix::fd_t, Event::Flags_t)>;
  struct callback_info_t
  {
    Event::Flags_t flags;
    callback_t function;
  };

  extern bool add(posix::fd_t target, Event::Flags_t flags, callback_t cb) noexcept; // add FD or process events to montior
  extern bool remove(posix::fd_t target, Event::Flags_t flags) noexcept; // remove from watch queue

  extern bool poll(int timeout = -1) noexcept;

  extern lockable<std::unordered_multimap<posix::fd_t, callback_info_t>> queue; // watch queue
  extern std::list<std::pair<posix::fd_t, Event::Flags_t>> results; // results from getevents()

  struct platform_dependant;
  extern struct platform_dependant s_platform;
}

#endif // EVENTBACKEND_H
