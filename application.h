#ifndef APPLICATION_H
#define APPLICATION_H

#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

template<typename T>
struct lockable : T, std::mutex
  { template<typename... ArgTypes> inline lockable(ArgTypes... args) : T(args...) { } };

template<typename... ArgTypes>
static void invoke(std::function<void(ArgTypes...)> f, ArgTypes... args) { f(args...); }

using vfunc = std::function<void()>;

class Application
{
public:
  Application(void);
 ~Application(void);

  int exec(void);

  static void quit(int return_value = 0);

private:
  static void wait_for_queue(void);
  static std::condition_variable m_step_exec;
  static lockable<std::queue<vfunc>> m_signal_queue;
  friend class Object;
};

#endif // APPLICATION_H
