#ifndef APPLICATION_H
#define APPLICATION_H

// STL
#include <queue>
#include <functional>

// PUT
#include <cxxutils/posix_helpers.h>
#include <specialized/mutex.h>
#include <specialized/eventbackend.h>


using vfunc = std::function<void()>;

class Application
{
public:
  Application(void) noexcept;

  int exec(void) noexcept;
  static void quit(int return_value = posix::success_response) noexcept;

private:
  static void step(void) noexcept;
  static void read(posix::fd_t fd, native_flags_t) noexcept;
  static posix::lockable<std::queue<vfunc>> ms_signal_queue;
  friend class Object;
};

#endif // APPLICATION_H
