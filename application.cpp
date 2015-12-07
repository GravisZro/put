#include "application.h"

#include <thread>

// === private code ===

std::condition_variable     Application::m_step_exec;
lockable<std::queue<vfunc>> Application::m_signal_queue;


// === inaccessible code ===

static bool s_run = true;
static int  s_return_value = 0;
static std::condition_variable s_queue_wait;

static bool is_main_thread(void)
{
  static std::thread::id main_id = std::this_thread::get_id();
  return main_id == std::this_thread::get_id();
}


// === public code ===

Application::Application (void) { }
Application::~Application(void) { }

int Application::exec(void)
{
  std::mutex m;
  while(s_run)
  {
    std::unique_lock<std::mutex> lk(m);
    m_step_exec.wait(lk, [] { return !m_signal_queue.empty(); } );

    m_signal_queue.lock(); // multithread protection
    while(m_signal_queue.size())
    {
      invoke(m_signal_queue.back());
      m_signal_queue.pop();
    }
    s_queue_wait.notify_all();
    m_signal_queue.unlock();
  }
  return s_return_value;
}

void Application::quit(int return_value)
{
  if(is_main_thread()) // if in main thread
  {
    while(m_signal_queue.size() > 1) // remove all but one from the queue
      m_signal_queue.pop();
  }
  else // if in secondary thread
  {
    m_signal_queue.lock(); // multithread protection
    while(m_signal_queue.size()) // wipe the queue
      m_signal_queue.pop();
    s_queue_wait.notify_all(); // notify all waiting objects it's ok to destruct
  }

  s_return_value = return_value;
  s_run = false;
}


// === private code ===

void Application::wait_for_queue(void)
{
  if(!is_main_thread())
  {
    std::mutex m;
    std::unique_lock<std::mutex> lk(m);
    m_step_exec.notify_one(); // tell Application to process the signal queue
    s_queue_wait.wait(lk, [] { return m_signal_queue.empty(); } ); // wait for queue to be empty
  }
}
