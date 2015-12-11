#ifndef OBJECT_H
#define OBJECT_H

#include "application.h"
#include <thread>
#include <functional>

struct ProtoObject
{
  inline  ProtoObject(void) { self = this; }
  inline ~ProtoObject(void) { self = nullptr; }
  ProtoObject* self;
};

class Object : private ProtoObject
{
public:
  template<typename... ArgTypes>
  struct signal
  {
    std::function<void(ProtoObject*, ArgTypes...)> func;
    ProtoObject* obj;
  };

  inline Object(void)  { }
  inline ~Object(void) { }

  template<class ObjType, typename SlotType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, SlotType&& slot)
  {
    sig.obj = obj;
    sig.func = [slot](ProtoObject* p, ArgTypes... args)
      { if(p == p->self) (static_cast<ObjType*>(p)->*slot)(args...); };
  }

  template<typename... ArgTypes>
  static inline void queue(signal<ArgTypes...>& sig, ArgTypes... args)
  {
    std::lock_guard<lockable<std::queue<vfunc>>> lock(Application::m_signal_queue); // multithread protection
    Application::m_signal_queue.emplace(std::bind(sig.func, sig.obj, args...));
    Application::m_step_exec.notify_one(); // inform execution stepper
  }
};

#endif // OBJECT_H
