#include "application.h"

// POSIX
#include <stropts.h>

// STL
#include <atomic>
#include <cassert>
#include <algorithm>

// PDTK
#include <object.h>
#include <specialized/eventbackend.h>

// atomic vars are to avoid race conditions
static std::atomic_int  s_return_value (0);
static std::atomic_bool s_run (true); // quit signal
static posix::fd_t s_pipeio[2] = { posix::invalid_descriptor }; //  execution stepper pipe

lockable<std::queue<vfunc>> Application::ms_signal_queue;
std::unordered_multimap<posix::fd_t, std::pair<EventFlags_t, vfdfunc>> Application::ms_fd_signals;

enum {
  Read = 0,
  Write = 1,
};

Application::Application (void) noexcept
{
  if(s_pipeio[Read] == posix::invalid_descriptor) // if execution stepper pipe  hasn't been initialized yet
  {
    flaw(::pipe(s_pipeio) == posix::error_response, posix::critical, ::exit(1),,
         "Unable to create pipe for execution stepper.")
    EventBackend::init(); // initialize event backend
    EventBackend::watch(s_pipeio[Read], EventFlags::Readable); // watch for when execution stepper pipe has content to read
  }
}

Application::~Application(void) noexcept
{
  if(s_pipeio[Read] != posix::invalid_descriptor)
  {
    EventBackend::destroy(); // shutdown event backend
    posix::close(s_pipeio[Read ]);
    posix::close(s_pipeio[Write]);
    s_pipeio[Read ] = posix::invalid_descriptor;
    s_pipeio[Write] = posix::invalid_descriptor;
  }
}

void Application::step(void) noexcept
{
  static uint8_t dummydata = 0; // dummy content
  posix::write(s_pipeio[Write], &dummydata, 1); // triggers run() to execute
}

int Application::exec(void) noexcept // non-static function to ensure an instance of Application exists
{
  while(s_run) // while not quitting
  {
    EventBackend::getevents(); // get event queue
    for(const std::pair<posix::fd_t, EventData_t> pos : EventBackend::results) // process queued events
    {
      if(pos.first == s_pipeio[Read]) // if this was object enqueue FD
      {
        while(::ioctl(pos.first, I_FLUSH, FLUSHRW) == posix::error_response && // discard the data (it's merely a trigger)
              errno == std::errc::interrupted); // don't be interrupted while flushing the FD channel
        // execute queue of object signal calls
        static std::queue<vfunc> exec_queue;
        if(exec_queue.empty()) // if not currently executing (recursive or multithread exec() calls?)
        {
          ms_signal_queue.lock(); // get exclusive access
          exec_queue.swap(ms_signal_queue); // swap the queues to prevent race conditions and the need for constant locking/unlocking
          ms_signal_queue.unlock(); // access is no longer needed

          while(!exec_queue.empty()) // while still have object signals to execute
          {
            exec_queue.front()(); // execute current object signal/callback
            exec_queue.pop(); // discard current object signal
          }
        }
      }
      else // if this was a watched FD
      {
        auto entries = ms_fd_signals.equal_range(pos.first); // get all the callback entries for that FD
        for_each(entries.first, entries.second, // for each FD
          [&pos](const std::pair<posix::fd_t, std::pair<EventFlags_t, vfdfunc>>& pair) // executed for each FD signal pair
            { pair.second.second(pos.first, pos.second); }); // call the fuction with the FD and triggering EventFlag
      }
    }
  }
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
