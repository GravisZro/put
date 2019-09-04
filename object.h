#ifndef OBJECT_H
#define OBJECT_H

// PUT
#include <put/cxxutils/posix_helpers.h>
#include <put/application.h>

// STL
#include <functional>
#include <map>
#include <new>

struct ProtoObject
{
  inline  ProtoObject(void) noexcept : self(this) { }
  inline ~ProtoObject(void) noexcept
  {
    if(valid())
      posix::Signal::raise(posix::Signal::MemoryBusError);
  }
  inline bool valid(void) const { return this == self; }
  void* self; // used to determine if type is soon to be be deleted
};


class Object : private ProtoObject
{
public:
  template<class ObjType, typename RType, typename... ArgTypes>
  using mslot_t = RType(ObjType::*)(ArgTypes...); // member slot

  template<typename RType, typename... ArgTypes>
  using fslot_t = std::function<RType(ArgTypes...) noexcept>; // function slot

  template<typename RType, typename... ArgTypes>
  using fpslot_t = RType(*)(ArgTypes...); // function pointer slot

  template<typename... ArgTypes>
  using signal_storage_t = std::multimap<ProtoObject*, fslot_t<void, ProtoObject*, ArgTypes...>>;

  template<typename... ArgTypes>
  struct signal : signal_storage_t<ArgTypes...>
  {
    typedef signal_storage_t<ArgTypes...> storage_t;

//    inline  signal(void) noexcept = default;
//    inline ~signal(void) noexcept = default;

    bool invocation(ArgTypes&... args) noexcept
    {
      if(!storage_t::empty() &&  // ensure that invalid signals are ignored
         Application::ms_signal_queue.lock()) // multithread protection
      {
        for(auto pos = storage_t::begin(); pos != storage_t::end();) // iterate through all connected slots
          if(pos->first == nullptr || pos->first->valid()) // ensure no object OR object is valid
          {
            Application::ms_signal_queue.emplace(std::bind(pos->second, pos->first, std::forward<ArgTypes>(args)...));
            ++pos;
          }
          else
            pos = storage_t::erase(pos);

        Application::step(); // inform execution stepper
        return Application::ms_signal_queue.unlock();
      }
      return false;
    }

    // connect to a member of an object
    template<class ObjType, typename RType>
    inline void connect(ObjType* obj, mslot_t<ObjType, RType, ArgTypes...> slot) noexcept
    {
      storage_t::emplace(static_cast<ProtoObject*>(obj),
        [slot](ProtoObject* p, ArgTypes... args) noexcept
          { if(p->valid()) (static_cast<ObjType*>(p)->*slot)(args...); }); // if ProtoObject is valid (not deleted), call slot
    }

    // connect to a function that accept the object pointer as the first argument
    template<class ObjType, typename RType>
    inline void connect(ObjType* obj, fslot_t<RType, ObjType*, ArgTypes...> slot) noexcept
    {
      storage_t::emplace(static_cast<ProtoObject*>(obj),
        [slot](ProtoObject* p, ArgTypes... args) noexcept
          { if(p->valid()) slot(static_cast<ObjType*>(p), args...); }); // if ProtoObject is valid (not deleted), call slot
    }

    // connect to a function and ignore the object
    template<typename RType>
    inline void connect(fslot_t<RType, ArgTypes...> slot) noexcept
    {
      storage_t::emplace(nullptr,
        [slot](ProtoObject*, ArgTypes... args) noexcept
          { slot(args...); });
    }

    // connect to another signal
    inline void connect(signal<ArgTypes...>& other) noexcept
    {
      storage_t::emplace(nullptr,
        [&other](ProtoObject*, ArgTypes... args) noexcept
          { other.invocation(args...); });
    }

    inline void disconnect(Object* obj) noexcept
      { storage_t::erase(obj); }

    inline void disconnect(void) noexcept
      { storage_t::clear(); }
  };


  inline  Object(void) noexcept = default;
  inline ~Object(void) noexcept = default;

  // connect to a member of an object
  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, mslot_t<ObjType, RType, ArgTypes...> slot) noexcept
    { sig.connect(obj, slot); }

  // connect to another signal
  template<typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig1, signal<ArgTypes...>& sig2) noexcept
    { sig1.connect(sig2); }

  // connect to a function that accept the object pointer as the first argument
  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, fslot_t<RType, ObjType*, ArgTypes...> slot) noexcept
    { sig.connect(obj, slot); }

  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, fpslot_t<RType, ObjType*, ArgTypes...> slot) noexcept
    { sig.connect(obj, fslot_t<RType, ArgTypes...>(slot)); }

  // connect to a lambda function
  template<typename Lambda, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, Lambda&& slot) noexcept
    { sig.connect(fslot_t<void, ArgTypes...>(slot)); }

  // connect to a function and ignore the object
  template<typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, fslot_t<RType, ArgTypes...> slot) noexcept
    { sig.connect(slot); }

  template<typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, fpslot_t<RType, ArgTypes...> slot) noexcept
    { sig.connect(fslot_t<RType, ArgTypes...>(slot)); }

  // disconnect all connections from signal
  template<typename... ArgTypes>
  static inline void disconnect(signal<ArgTypes...>& sig) noexcept
    { sig.disconnect(); }

  // disconnect all connections from signal to object
  template<typename... ArgTypes>
  static inline void disconnect(signal<ArgTypes...>& sig, Object* obj) noexcept
    { sig.disconnect(obj); }

  // enqueue a function call without a signal
  template<class ObjType, typename RType, typename... ArgTypes>
  static inline bool singleShot(ObjType* obj, mslot_t<ObjType, RType, ArgTypes...> slot, ArgTypes&... args) noexcept
  {
    if(obj->valid() && // ensure object is valid
       Application::ms_signal_queue.lock()) // multithread protection
    {
      Application::ms_signal_queue.emplace(std::bind(slot, obj, std::forward<ArgTypes>(args)...));
      Application::step(); // inform execution stepper
      return Application::ms_signal_queue.unlock();
    }
    return false;
  }

  template<typename RType, typename... ArgTypes>
  static inline bool singleShot(fslot_t<RType, ArgTypes...> slot, ArgTypes&... args) noexcept
  {
    if(Application::ms_signal_queue.lock()) // multithread protection
    {
      Application::ms_signal_queue.emplace(std::bind(slot, std::forward<ArgTypes>(args)...));
      Application::step(); // inform execution stepper
      return Application::ms_signal_queue.unlock();
    }
    return false;
  }

  template<typename RType, typename... ArgTypes>
  static inline void singleShot(fpslot_t<RType, ArgTypes...> slot, ArgTypes&... args) noexcept
    { singleShot(fslot_t<RType, ArgTypes...>(slot), args...);}

  // enqueue a call to the functions connected to the signal
  template<typename... ArgTypes>
  static inline bool enqueue(signal<ArgTypes...>& sig, ArgTypes&... args) noexcept
    { return sig.invocation(args...);}

  // enqueue a call to the functions connected to the signal with /copies/ of the arguments
  template<typename... ArgTypes>
  static inline bool enqueue_copy(signal<ArgTypes...>& sig, ArgTypes... args) noexcept
    { return sig.invocation(args...);}

  void operator delete(void* ptr) noexcept
  {
    static_cast<ProtoObject*>(ptr)->self = nullptr; // invalidate object
    if(Application::ms_signal_queue.lock()) // multithread protection
    {
      Application::ms_signal_queue.emplace([ptr](void) { ::operator delete(ptr); });
      Application::ms_signal_queue.unlock();
    }
  }
};

#endif // OBJECT_H
