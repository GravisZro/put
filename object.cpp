#include "object.h"

std::mutex                  Application::m_exec_step;
lockable<std::queue<vfunc>> Application::m_signal_queue;

Object::Object (void) { }
Object::~Object(void) { }

Application::Application (void) { }
Application::~Application(void) { }

int Application::exec(void)
{
  for(;;)
  {
    m_exec_step.lock();
    process_signal_queue();
  }
}

void Application::process_signal_queue(void)
{
  std::lock_guard<lockable<std::queue<vfunc>>> lock(Application::m_signal_queue); // multithread protection
  while(m_signal_queue.size())
  {
    invoke(m_signal_queue.back());
    m_signal_queue.pop();
  }
}
