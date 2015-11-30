#ifndef OBJECT_H
#define OBJECT_H

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

class Object
{
public:
  template<typename... ArgTypes>
  using signal = std::function<void(ArgTypes...)>;

  Object(void);
 ~Object(void);

  template<typename... ArgTypes>
    static void queue(signal<ArgTypes...>& sig, ArgTypes... args);

  template<class ObjType, typename SlotType>
    static void connect(signal<>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj); }

  template<class ObjType, typename SlotType, typename A1>
    static void connect(signal<A1>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj, _1); }

  template<class ObjType, typename SlotType, typename A1, typename A2>
    static void connect(signal<A1,A2>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj, _1, _2); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3>
    static void connect(signal<A1,A2,A3>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj, _1, _2, _3); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4>
    static void connect(signal<A1,A2,A3,A4>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj, _1, _2, _3, _4); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5>
    static void connect(signal<A1,A2,A3,A4,A5>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj, _1, _2, _3, _4, _5); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
    static void connect(signal<A1,A2,A3,A4,A5,A6>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj, _1, _2, _3, _4, _5, _6); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
    static void connect(signal<A1,A2,A3,A4,A5,A6,A7>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj, _1, _2, _3, _4, _5, _6, _7); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
    static void connect(signal<A1,A2,A3,A4,A5,A6,A7,A8>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj, _1, _2, _3, _4, _5, _6, _7, _8); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
    static void connect(signal<A1,A2,A3,A4,A5,A6,A7,A8,A9>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig = std::bind(slot, *obj, _1, _2, _3, _4, _5, _6, _7, _8, _9); }
};


class Application
{
public:
  Application(void);
 ~Application(void);

  int exec(void);

private:
  static std::condition_variable     m_step_exec;
  static lockable<std::queue<vfunc>> m_signal_queue;
  friend class Object;
};

#include <iostream>

template<typename... ArgTypes>
void Object::queue(signal<ArgTypes...>& sig, ArgTypes... args)
{
  std::lock_guard<lockable<std::queue<vfunc>>> lock(Application::m_signal_queue); // multithread protection
  if(sig != nullptr) // if signal is connected...
    Application::m_signal_queue.emplace(std::bind(invoke<ArgTypes...>, sig, args...));
  else
    std::cout << "not queuing!" << std::endl;
  Application::m_step_exec.notify_one(); // inform execution stepper
}
#endif // OBJECT_H
