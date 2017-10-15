#include "application.h"

// STL
#include <atomic>
#include <list>

// atomic vars are to avoid race conditions
static std::atomic_int  s_return_value (0);
static std::atomic_bool s_run (true); // quit signal

lockable<std::queue<vfunc>> Application::ms_signal_queue;

Application::Application(void) noexcept
{
  if(!EventBackend::setstepper(stepper))
    quit(posix::error_response);
}

int Application::exec(void) noexcept // non-static function to ensure an instance of Application exists
{
  while(s_run) // while not quitting
  {
    EventBackend::poll(); // get event queue results

    for(std::pair<posix::fd_t, native_flags_t>& pair : EventBackend::results) // process results
    {
      EventBackend::queue.lock(); // get exclusive access (make thread-safe)
      auto entries = EventBackend::queue.equal_range(pair.first); // get all the entries for that FD
      std::list<std::pair<posix::fd_t, EventBackend::callback_info_t>> exec_fds(entries.first, entries.second); // copy entries
      EventBackend::queue.unlock(); // access is no longer needed

      for(std::pair<posix::fd_t, EventBackend::callback_info_t>& entry : exec_fds) // for each FD
        if(entry.second.flags & pair.second) // check if there is a matching flag
          entry.second.function(pair.first, pair.second); // invoke the callback function with the FD and triggering Flag
    }
  }
  return s_return_value; // quit() has been called, return value specified
}

// this is the callback function for the signal queue
void Application::stepper(void) noexcept
{
  // execute queue of object signal calls
  static std::queue<vfunc> exec_queue;
  if(exec_queue.empty()) // if not currently executing (recursive or multithread exec() calls?)
  {
    ms_signal_queue.lock(); // get exclusive access (make thread-safe)
    exec_queue.swap(ms_signal_queue); // swap the queues
    ms_signal_queue.unlock(); // access is no longer needed

    while(!exec_queue.empty()) // while still have object signals to execute
    {
      exec_queue.front()(); // execute current object signal/callback
      exec_queue.pop(); // discard current object signal
    }
  }
}

void Application::quit(int return_value) noexcept // soft application exit (allows event queues to complete)
{
  if(s_run) // if not already quitting
  {
    s_return_value = return_value; // set application return value
    s_run = false; // indicate program must quit
  }
}
