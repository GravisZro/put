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

typedef uint64_t native_flags_t;
using vfunc = std::function<void()>;

namespace EventBackend
{
  using callback_t = std::function<void(posix::fd_t, native_flags_t) noexcept>;
  struct callback_info_t
  {
    native_flags_t flags;
    callback_t function;
  };

  extern void step(void) noexcept;
  extern bool setstepper(vfunc function) noexcept;

  extern bool add(posix::fd_t target, native_flags_t flags, callback_t function) noexcept; // add FD or process events to montior
  extern bool remove(posix::fd_t target, native_flags_t flags) noexcept; // remove from watch queue

  extern bool poll(int timeout = -1) noexcept;

  extern lockable<std::unordered_multimap<posix::fd_t, callback_info_t>> queue; // watch queue
  extern std::list<std::pair<posix::fd_t, native_flags_t>> results; // results from getevents()

  struct platform_dependant;
  extern struct platform_dependant s_platform;
}

#endif // EVENTBACKEND_H
