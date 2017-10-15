#ifndef APPLICATION_H
#define APPLICATION_H

// STL
#include <queue>

// PDTK
#include <cxxutils/posix_helpers.h>
#include <specialized/eventbackend.h>



class Application
{
public:
  Application(void) noexcept;

  int exec(void) noexcept;
  static void quit(int return_value = posix::success_response) noexcept;

private:
  static void stepper(void) noexcept;
  static lockable<std::queue<vfunc>> ms_signal_queue;
  friend class Object;
};

#endif // APPLICATION_H
