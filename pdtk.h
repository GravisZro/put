#ifndef PDTK_H
#define PDTK_H

#include <queue>
#include <functional>
#include <thread>
#include <mutex>

template<typename... Args>
static void invoke(std::function<void(Args...)> f, Args... args) { f(args...); }

template<typename... Args>
using signal = std::function<void(Args...)>;
using vfunc  = std::function<void(       )>;


class Object
{
public:
  Object(void);
 ~Object(void);

protected:
  template<typename... Args>
    static void queue(signal<Args...>& sig, Args... args);

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
  static void process_signal_queue(void);

private:
  static std::mutex m_exec_mutex;
  static std::queue<vfunc> m_signal_queue;
  friend class Object;
};

template<typename... Args>
void Object::queue(signal<Args...>& sig, Args... args)
{
  Application::m_signal_queue.emplace(std::bind(invoke<Args...>, sig, args...));
  Application::m_exec_mutex.unlock();
}

#endif // PDTK_H
