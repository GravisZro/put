#ifndef OBJECT_H
#define OBJECT_H

#include <application.h>
#include <thread>
#include <functional>

struct ProtoObject
{
  inline  ProtoObject(void) noexcept { self = this; }
  inline ~ProtoObject(void) noexcept { self = nullptr; }
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

  inline Object(void) noexcept  { }
  inline ~Object(void) noexcept { }

  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, RType(ObjType::*slot)(ArgTypes...)) noexcept
  {
    sig.obj = obj;
    sig.func = [slot](ProtoObject* p, ArgTypes... args) noexcept
      { if(p == p->self) (static_cast<ObjType*>(p)->*slot)(args...); }; // if ProtoObject is valid (not deleted), call slot
  }

  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, RType(*slot)(ObjType*, ArgTypes...)) noexcept
  {
    sig.obj = obj;
    sig.func = [slot](ProtoObject* p, ArgTypes... args) noexcept
      { if(p == p->self) slot(static_cast<ObjType*>(p), args...); }; // if ProtoObject is valid (not deleted), call slot
  }

  template<typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, RType(*slot)(ArgTypes...)) noexcept
    { sig.func = [slot](ProtoObject*, ArgTypes... args) noexcept { slot(args...); }; }

  template<typename... ArgTypes>
  static inline bool enqueue(signal<ArgTypes...>& sig, ArgTypes&... args) noexcept
  {
    if(sig.func != nullptr) // ensure that invalid signals are not enqueued
    {
      std::lock_guard<lockable<std::queue<vfunc>>> lock(Application::m_signal_queue); // multithread protection
      Application::m_signal_queue.emplace(std::bind(sig.func, sig.obj, std::forward<ArgTypes>(args)...));
      Application::m_step_exec.notify_one(); // inform execution stepper
      return true;
    }
    return false;
  }

  template<typename... ArgTypes>
  static inline bool enqueue_copy(signal<ArgTypes...>& sig, ArgTypes... args) noexcept
    { return enqueue(sig, args...);}
};

#endif // OBJECT_H
