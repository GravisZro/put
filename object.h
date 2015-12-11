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
    void(*func)(ProtoObject*, ArgTypes...);
    ProtoObject* obj;
  };

  inline Object(void)  { }
  inline ~Object(void) { }

  template<class ObjType, typename SlotType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, SlotType&& slot)
  {
    sig.obj = obj;
    sig.func = reinterpret_cast<void(*)(ProtoObject*, ArgTypes...)>(slot);
    // note: add -Wno-pmf-conversions if you are getting warnings
  }

  template<typename... ArgTypes>
  static inline void queue(signal<ArgTypes...>& sig, ArgTypes... args)
  {
    std::lock_guard<lockable<std::queue<vfunc_pair>>> lock(Application::m_signal_queue); // multithread protection
    Application::m_signal_queue.emplace(std::bind(sig.func, sig.obj, args...), sig.obj);
    Application::m_step_exec.notify_one(); // inform execution stepper
  }
};

#endif // OBJECT_H
