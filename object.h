#ifndef OBJECT_H
#define OBJECT_H

// PUT
#include <cxxutils/posix_helpers.h>
#include <application.h>

// STL
#include <functional>
#include <map>

struct ProtoObject
{
  inline  ProtoObject(void) noexcept { self = this; }
  inline ~ProtoObject(void) noexcept { self = nullptr; }
  ProtoObject* self; // used to determine if type has been deleted
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
  using signal = std::multimap<ProtoObject*, fslot_t<void, ProtoObject*, ArgTypes...>>;

  inline  Object(void) noexcept = default;
  inline ~Object(void) noexcept = default;

  // connect to a member of an object
  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, mslot_t<ObjType, RType, ArgTypes...> slot) noexcept
  {
    sig.emplace(static_cast<ProtoObject*>(obj),
      [slot](ProtoObject* p, ArgTypes... args) noexcept
        { if(p == p->self) (static_cast<ObjType*>(p)->*slot)(args...); }); // if ProtoObject is valid (not deleted), call slot
  }

  // connect to another signal
  template<typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig1, signal<ArgTypes...>& sig2) noexcept
  {
    sig1.emplace(static_cast<ProtoObject*>(nullptr),
      [&sig2](ProtoObject*, ArgTypes... args) noexcept
        { enqueue(sig2, args...); });
  }

  // connect to a function that accept the object pointer as the first argument
  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, fslot_t<RType, ObjType*, ArgTypes...>&& slot) noexcept
  {
    sig.emplace(static_cast<ProtoObject*>(obj),
      [slot](ProtoObject* p, ArgTypes... args) noexcept
        { if(p == p->self) slot(static_cast<ObjType*>(p), args...); }); // if ProtoObject is valid (not deleted), call slot
  }

  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, fpslot_t<RType, ObjType*, ArgTypes...> slot) noexcept
    { connect(sig, obj, fslot_t<RType, ArgTypes...>(slot)); }

  // connect to a function and ignore the object
  template<typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, fslot_t<RType, ArgTypes...>&& slot) noexcept
  {
    sig.emplace(static_cast<ProtoObject*>(nullptr),
      [slot](ProtoObject*, ArgTypes... args) noexcept
        { slot(args...); });
  }

  template<typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, fpslot_t<RType, ArgTypes...> slot) noexcept
    { connect(sig, fslot_t<RType, ArgTypes...>(slot)); }


  // disconnect all connections from signal
  template<typename... ArgTypes>
  static inline void disconnect(signal<ArgTypes...>& sig) noexcept
    { sig.clear(); }

  // disconnect all connections from signal to object
  template<typename... ArgTypes>
  static inline void disconnect(signal<ArgTypes...>& sig, Object* obj) noexcept
    { sig.erase(obj); }

  // enqueue a function call without a signal
  template<class ObjType, typename RType, typename... ArgTypes>
  static inline bool singleShot(ObjType* obj, mslot_t<ObjType, RType, ArgTypes...> slot, ArgTypes&... args) noexcept
  {
    if(Application::ms_signal_queue.lock()) // multithread protection
    {
      Application::ms_signal_queue.emplace(std::bind(slot, obj, std::forward<ArgTypes>(args)...));
      Application::step(); // inform execution stepper
      return Application::ms_signal_queue.unlock();
    }
    return false;
  }

  template<typename RType, typename... ArgTypes>
  static inline bool singleShot(fslot_t<RType, ArgTypes...>&& slot, ArgTypes&... args) noexcept
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
  static inline bool enqueue(const signal<ArgTypes...>& sig, ArgTypes&... args) noexcept
    { return enqueue_private(sig, args...);}

  // enqueue a call to the functions connected to the signal with /copies/ of the arguments
  template<typename... ArgTypes>
  static inline bool enqueue_copy(const signal<ArgTypes...>& sig, ArgTypes... args) noexcept
    { return enqueue_private(sig, args...);}

private:
  template<class signal_type, typename... ArgTypes>
  static inline bool enqueue_private(const signal_type& sig, ArgTypes... args) noexcept
  {
    if(!sig.empty() &&  // ensure that invalid signals are not enqueued
       Application::ms_signal_queue.lock()) // multithread protection
    {
      for(auto sigpair : sig) // iterate through all connected slots
        Application::ms_signal_queue.emplace(std::bind(sigpair.second, sigpair.first, std::forward<ArgTypes>(args)...));
      Application::step(); // inform execution stepper
      return Application::ms_signal_queue.unlock();
    }
    return false;
  }
};

#endif // OBJECT_H
