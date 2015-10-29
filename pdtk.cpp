#include "pdtk.h"

std::queue<vfunc> Application::m_signal_queue;
std::mutex        Application::m_exec_mutex;

Object::Object (void) { }
Object::~Object(void) { }



Application::Application (void) { }
Application::~Application(void) { }

int Application::exec(void)
{
  for(;;)
  {
    m_exec_mutex.lock();
    process_signal_queue();
  }
}

void Application::process_signal_queue(void)
{
  while(m_signal_queue.size())
  {
    invoke(m_signal_queue.back());
    m_signal_queue.pop();
  }
}
