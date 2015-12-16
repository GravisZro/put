#include "application.h"
#include "object.h"

#include <atomic>

static std::atomic_int  s_return_value (0);
static std::atomic_bool s_run (true);

std::condition_variable     Application::m_step_exec;
lockable<std::queue<vfunc>> Application::m_signal_queue;

Application::Application (void) { }
Application::~Application(void) { }

int Application::exec(void)
{
  while(s_run)
  {
    std::unique_lock<std::mutex> lk(m_signal_queue); // multithread protection
    m_step_exec.wait(lk, [] { return !m_signal_queue.empty(); } ); // wait for notify_one() call and non-empty queue

    while(s_run && m_signal_queue.size())
    {
      m_signal_queue.front()();
      m_signal_queue.pop();
    }
  }
  return s_return_value;
}

void Application::quit(int return_value)
{
  if(s_run)
  {
    s_return_value = return_value;
    s_run = false;
  }
}
