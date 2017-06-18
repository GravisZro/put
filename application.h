#ifndef APPLICATION_H
#define APPLICATION_H

// STL
#include <queue>
#include <functional>
#include <mutex>
#include <unordered_map>

// PDTK
#include <cxxutils/posix_helpers.h>
#include <specialized/eventbackend.h>

template<typename T>
struct lockable : T, std::mutex
  { template<typename... ArgTypes> constexpr lockable(ArgTypes... args) noexcept : T(args...) { } };
using vfunc = std::function<void()>;
using vfdfunc = std::function<void(posix::fd_t, EventData_t)>;

class Application
{
public:
  Application(void) noexcept;
 ~Application(void) noexcept;

  int exec(void) noexcept;

  static void quit(int return_value = 0) noexcept;

private:
  static void step(void) noexcept;
  static lockable<std::queue<vfunc>> ms_signal_queue;
  static std::unordered_multimap<posix::fd_t, std::pair<EventFlags_t, vfdfunc>> ms_fd_signals;
  friend class Object;
};

#endif // APPLICATION_H
