#include "application.h"

// STL
#include <atomic>

// PDTK
#include "object.h"


static std::atomic_int  s_return_value (0);
static std::atomic_bool s_run (true);

std::condition_variable     Application::m_step_exec;
lockable<std::queue<vfunc>> Application::m_signal_queue;

Application::Application (void) noexcept { }
Application::~Application(void) noexcept { }

int Application::exec(void) noexcept
{
  static std::queue<vfunc> exec_queue;
  static std::mutex exec_mutex;
  while(s_run)
  {
    std::unique_lock<std::mutex> lk(exec_mutex); // auto lock/unlock
    m_step_exec.wait(lk, [] { return !m_signal_queue.empty(); } ); // wait for notify_one() call and non-empty queue

    m_signal_queue.lock();
    exec_queue.swap(m_signal_queue);
    m_signal_queue.unlock();

    while(s_run && !exec_queue.empty())
    {
      exec_queue.front()();
      exec_queue.pop();
    }
  }
  return s_return_value;
}

void Application::quit(int return_value) noexcept
{
  if(s_run)
  {
    s_return_value = return_value;
    s_run = false;
  }
}
