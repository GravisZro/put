#ifndef APPLICATION_H
#define APPLICATION_H

// STL
#include <queue>
#include <functional>

// PDTK
#include <cxxutils/posix_helpers.h>
#include <specialized/eventbackend.h>

using vfunc = std::function<void()>;

class Application
{
public:
  Application(void) noexcept;
 ~Application(void) noexcept;

  int exec(void) noexcept;

  static void quit(int return_value = posix::success_response) noexcept;

private:
  static void read(posix::fd_t fd, Event::Flags_t flags);
  static void step(void) noexcept;
  static lockable<std::queue<vfunc>> ms_signal_queue;
  friend class Object;
};

#endif // APPLICATION_H
