#include "object.h"

std::condition_variable     Application::m_step_exec;
lockable<std::queue<vfunc>> Application::m_signal_queue;

Object::Object (void) { }
Object::~Object(void) { }

Application::Application (void) { }
Application::~Application(void) { }

int Application::exec(void)
{
  std::mutex m;
  for(;;)
  {
    std::unique_lock<std::mutex> lk(m);
    m_step_exec.wait(lk, [] { return !m_signal_queue.empty(); } );

    m_signal_queue.lock(); // multithread protection
    while(m_signal_queue.size())
    {
      invoke(m_signal_queue.back());
      m_signal_queue.pop();
    }
    m_signal_queue.unlock();
  }
}
