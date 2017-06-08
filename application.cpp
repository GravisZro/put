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
static std::atomic_bool s_run (true);
static posix::fd_t s_pipeio[2] = { posix::invalid_descriptor };

lockable<std::queue<vfunc>> Application::ms_signal_queue;
std::unordered_multimap<posix::fd_t, std::pair<EventFlags_t, vfdfunc>> Application::ms_fd_signals;

enum {
  Read = 0,
  Write = 1,
};

Application::Application (void) noexcept
{
  if(s_pipeio[Read] == posix::invalid_descriptor)
  {
    assert(::pipe(s_pipeio) != posix::error_response);
    EventBackend::init();
    EventBackend::watch(s_pipeio[Read], EventFlags::Readable);
  }
}

Application::~Application(void) noexcept
{
  EventBackend::destroy();
}

void Application::step(void) noexcept
{
  static uint8_t dummydata = 0;
  posix::write(s_pipeio[Write], &dummydata, 1);
}

int Application::exec(void) noexcept // non-static function to ensure an instance of Application exists
{
  while(s_run)
  {
    EventBackend::getevents();
    for(auto pos : EventBackend::results)
    {
      if(pos.first == s_pipeio[Read])
      {
        while(::ioctl(pos.first, I_FLUSH, FLUSHRW) == posix::error_response && // not interested in the data
              errno == std::errc::interrupted); // ignore interruptions
        run();
      }
      else
      {
        auto entries = ms_fd_signals.equal_range(pos.first);
        for_each(entries.first, entries.second,
          [pos](auto& entry)
        {
          if(entry.second.first & pos.second)
            entry.second.second(pos.first, pos.second);
        });
      }
    }
  }
  return s_return_value; // quit() has been called, return value specified
}

void Application::run(void) noexcept
{
  static std::queue<vfunc> exec_queue;
  if(s_run && exec_queue.empty()) // while application is running and not currently executing
  {
    ms_signal_queue.lock(); // get exclusive access
    exec_queue.swap(ms_signal_queue); // swap the queues to prevent race conditions and the need for constant locking/unlocking
    ms_signal_queue.unlock(); // access is no longer needed

    while(s_run && !exec_queue.empty()) // if haven't quit and still have signals to execute
    {
      exec_queue.front()(); // execute current signal
      exec_queue.pop(); // discard current signal
    }
  }
}

void Application::quit(int return_value) noexcept
{
  if(s_run)
  {
    s_return_value = return_value;
    s_run = false;
  }
}
