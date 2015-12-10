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
    using func_t = void(*)(ProtoObject*, ArgTypes...);

    inline bool valid(void)
      { return func != nullptr && obj != nullptr && obj == obj->self; }

    template<typename ObjType, typename SlotType>
    inline void bind(ObjType* object, SlotType&& slot)
      { obj = object; func = reinterpret_cast<func_t>(slot); }

    func_t func;
    ProtoObject* obj;
  };

  inline Object(void)  { }
  inline ~Object(void) { }

  template<class ObjType, typename SlotType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, SlotType&& slot)
    { sig.bind(obj, slot); }

  template<typename... ArgTypes>
  static void queue(signal<ArgTypes...>& sig, ArgTypes... args)
  {
    if(sig.valid()) // if signal is connected...
    {
      std::lock_guard<lockable<std::queue<vfunc_pair>>> lock(Application::m_signal_queue); // multithread protection
      Application::m_signal_queue.emplace(std::bind(sig.func, sig.obj, args...), sig.obj);
      Application::m_step_exec.notify_one(); // inform execution stepper
    };
  }
};

#endif // OBJECT_H
