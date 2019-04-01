#include "application.h"

// STL
#include <atomic>
#include <list>

// PUT
#include <put/cxxutils/vterm.h>

// atomic vars are to avoid race conditions
static std::atomic_int  s_return_value(0);
static std::atomic_bool s_run(true); // quit signal

posix::lockable<std::queue<vfunc>> Application::ms_signal_queue;

static posix::fd_t s_pipeio[2] = { posix::invalid_descriptor }; //  execution stepper pipe

enum {
  Read = 0,
  Write = 1,
};

Application::Application(void) noexcept
{
  if(s_pipeio[Read] == posix::invalid_descriptor) // if execution stepper pipe  hasn't been initialized yet
  {
    flaw(!posix::pipe(s_pipeio),
         terminal::critical,
         posix::exit(errno),,
         "Unable to create pipe for execution stepper: %s", posix::strerror(errno))

    posix::fcntl(s_pipeio[Read ], F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::fcntl(s_pipeio[Write], F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::donotblock(s_pipeio[Read]); // don't block

    flaw(!EventBackend::add(s_pipeio[Read], EventBackend::SimplePollReadFlags, read),
         terminal::critical,
         posix::exit(errno),, // watch for when execution stepper pipe has been triggered
         "Unable to watch execution stepper to backend: %s", posix::strerror(errno))
  }
}

void Application::step(void) noexcept
{
  static const uint8_t dummydata = 0; // dummy content
  flaw(posix::write(s_pipeio[Write], &dummydata, 1) != 1,
       terminal::critical,
       posix::exit(errno),, // triggers execution stepper FD
       "Unable to trigger Object signal queue processor: %s", posix::strerror(errno))
}

// this is the callback function for the signal queue
void Application::read(posix::fd_t fd, native_flags_t) noexcept
{
  uint64_t discard;
  while(posix::read(fd, &discard, sizeof(discard)) != posix::error_response);

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

void Application::processQueue(void) noexcept
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

int Application::exec(void) noexcept // non-static function to ensure an instance of Application exists
{
  s_run = true;
  while(s_run) // while not quitting
    processQueue();
  return s_return_value; // quit() has been called, return value specified
}

void Application::quit(int return_value) noexcept // soft application exit (allows event queues to complete)
{
  if(s_run) // if not already quitting
  {
    s_return_value = return_value; // set application return value
    s_run = false; // indicate program must quit
  }
}
