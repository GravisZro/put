#ifndef OBJECT_H
#define OBJECT_H

#include <functional>
#include "application.h"

#include <thread>
#include <functional>

#include <iostream>

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
    inline bool valid(void)
      { return func != nullptr && obj != nullptr && obj == obj->self; }

    template<typename ObjType, typename SlotType, typename... _ArgTypes>
    void bind(ObjType* object, SlotType&& slot, _ArgTypes... args)
    {
      obj = object;
      func = std::bind(slot, std::ref(*object), args...);
    }

    std::function<void(ArgTypes...)> func;
    ProtoObject* obj;
  };

  inline Object(void)  { }
  inline ~Object(void) { }

  template<typename... ArgTypes>
    static void queue(signal<ArgTypes...>& sig, ArgTypes... args);

#if 0
  template<class ObjType, typename SlotType, typename... ArgTypes>
    static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, SlotType&& slot)
      { sig.bind(obj, slot); }
#else

  template<class ObjType, typename SlotType>
    static inline void connect(signal<>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot); }

  template<class ObjType, typename SlotType, typename A1>
    static inline void connect(signal<A1>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot, _1); }

  template<class ObjType, typename SlotType, typename A1, typename A2>
    static inline void connect(signal<A1,A2>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot, _1, _2); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3>
    static inline void connect(signal<A1,A2,A3>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot, _1, _2, _3); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4>
    static inline void connect(signal<A1,A2,A3,A4>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot, _1, _2, _3, _4); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5>
    static inline void connect(signal<A1,A2,A3,A4,A5>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot, _1, _2, _3, _4, _5); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
    static inline void connect(signal<A1,A2,A3,A4,A5,A6>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot, _1, _2, _3, _4, _5, _6); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
    static inline void connect(signal<A1,A2,A3,A4,A5,A6,A7>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot, _1, _2, _3, _4, _5, _6, _7); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
    static inline void connect(signal<A1,A2,A3,A4,A5,A6,A7,A8>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot, _1, _2, _3, _4, _5, _6, _7, _8); }

  template<class ObjType, typename SlotType, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
    static inline void connect(signal<A1,A2,A3,A4,A5,A6,A7,A8,A9>& sig, ObjType* obj, SlotType&& slot)
      { using namespace std::placeholders; sig.bind(obj, slot, _1, _2, _3, _4, _5, _6, _7, _8, _9); }
#endif
};

template<typename... ArgTypes>
void Object::queue(signal<ArgTypes...>& sig, ArgTypes... args)
{
  if(sig.valid()) // if signal is connected...
  {
    std::lock_guard<lockable<std::queue<vfunc_pair>>> lock(Application::m_signal_queue); // multithread protection
    Application::m_signal_queue.emplace(std::bind(invoke<ArgTypes...>, sig.func, args...), sig.obj);
    Application::m_step_exec.notify_one(); // inform execution stepper
  }
}

#endif // OBJECT_H
