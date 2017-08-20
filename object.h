#ifndef OBJECT_H
#define OBJECT_H

// PDTK
#include <cxxutils/posix_helpers.h>
#include <application.h>

// STL
#include <functional>
#include <unordered_map>

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
  using fslot_t = RType(*)(ArgTypes...); // function slot

  template<typename... ArgTypes>
  using signal = std::unordered_multimap<ProtoObject*, std::function<void(ProtoObject*, ArgTypes...)>>;

  inline  Object(void) noexcept { }
  inline ~Object(void) noexcept { }

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
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, fslot_t<RType, ObjType*, ArgTypes...> slot) noexcept
  {
    sig.emplace(static_cast<ProtoObject*>(obj),
      [slot](ProtoObject* p, ArgTypes... args) noexcept
        { if(p == p->self) slot(static_cast<ObjType*>(p), args...); }); // if ProtoObject is valid (not deleted), call slot
  }

  // connect to a function and ignore the object
  template<typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, fslot_t<RType, ArgTypes...> slot) noexcept
  {
    sig.emplace(static_cast<ProtoObject*>(nullptr),
      [slot](ProtoObject*, ArgTypes... args) noexcept
        { slot(args...); });
  }

  // connect a file descriptor event to an object member function
  template<class ObjType, typename RType>
  static inline void connect(posix::fd_t fd, EventFlags_t flags, ObjType* obj, mslot_t<ObjType, RType, posix::fd_t, EventData_t> slot) noexcept
  {
    EventBackend::watch(fd, flags);
    Application::ms_fd_signals.emplace(fd, std::make_pair(flags,
      [obj, slot](posix::fd_t _fd, EventData_t data) noexcept
        { if(obj == obj->self) (obj->*slot)(_fd, data); })); // if ProtoObject is valid (not deleted), call slot
  }

  template<class ObjType, typename RType>
  static inline void connect(posix::fd_t fd, ObjType* obj, mslot_t<ObjType, RType, posix::fd_t, EventData_t> slot) noexcept
    { connect(fd, EventFlags::Readable, obj, slot); }

  // connect an file descriptor event to a signal
  static inline void connect(posix::fd_t fd, EventFlags_t flags, signal<posix::fd_t, EventData_t>& sig) noexcept
  {
    EventBackend::watch(fd, flags);
    Application::ms_fd_signals.emplace(fd, std::make_pair(flags,
      [&sig](posix::fd_t _fd, EventData_t _data) noexcept
      {
        if(!sig.empty()) // ensure that invalid signals are not enqueued
        {
          std::lock_guard<lockable<std::queue<vfunc>>> lock(Application::ms_signal_queue); // multithread protection
          for(auto sigpair : sig) // iterate through all connected slots
            Application::ms_signal_queue.emplace(std::bind(sigpair.second, sigpair.first, _fd, _data));
          Application::step(); // inform execution stepper
        }
      }));
  }

  static inline void connect(posix::fd_t fd, signal<posix::fd_t, EventData_t>& sig) noexcept
    { connect(fd, EventFlags::Readable, sig); }

  // connect a file descriptor event to a function
  template<typename RType>
  static inline void connect(posix::fd_t fd, EventFlags_t flags, fslot_t<RType, posix::fd_t, EventData_t> slot) noexcept
  {
    EventBackend::watch(fd, flags);
    Application::ms_fd_signals.emplace(fd, std::make_pair(flags,
      [slot](posix::fd_t fd, EventData_t data) noexcept
        { slot(fd, data); }));
  }

  template<typename RType>
  static inline void connect(posix::fd_t fd, fslot_t<RType, posix::fd_t, EventData_t> slot) noexcept
    { connect(fd, EventFlags::Readable, slot); }

  // connect a file location to a function
  template<typename RType>
  static inline void connect(const char* path, EventFlags_t flags, fslot_t<RType, posix::fd_t, EventData_t> slot) noexcept
  {
    posix::fd_t fd = EventBackend::watch(path, flags);
    Application::ms_fd_signals.emplace(fd, std::make_pair(flags,
      [slot](posix::fd_t fd, EventData_t data) noexcept
        { slot(fd, data); }));
  }

  template<typename RType>
  static inline void connect(const char* path, fslot_t<RType, posix::fd_t, EventData_t> slot) noexcept
    { connect(path, EventFlags::FileMod, slot); }

  // disconnect all connections from signal
  template<typename... ArgTypes>
  static inline void disconnect(signal<ArgTypes...>& sig) noexcept
    { sig.clear(); }

  // disconnect all connections from signal to object
  template<typename... ArgTypes>
  static inline void disconnect(signal<ArgTypes...>& sig, Object* obj) noexcept
    { sig.erase(obj); }

  // disconnect connections to file for all event flags
  static inline void disconnect(const char* path) noexcept
    { disconnect(EventBackend::lookup(path), EventFlags::FileEvent | EventFlags::DirEvent); }

  // disconnect connections to fd for certain event flags
  static inline void disconnect(posix::fd_t fd, EventFlags_t flags = EventFlags::Readable) noexcept
  {
    EventBackend::remove(fd, flags); // allowed to fail
    auto range = Application::ms_fd_signals.equal_range(fd);
    auto pos = range.first; // pos = iterator
    while(pos != range.second) // pos is _always_ advanced within loop
    {
      if(pos->second.first.isSet(flags)) // if the flags match partially
      {
        if(pos->second.first.unset(flags)) // remove all matching parts and return if any flags remain
          ++pos; // advance iterator
        else
          pos = Application::ms_fd_signals.erase(pos); // completely remove and advance iterator
      }
      else
        ++pos; // advance iterator
    }
  }

  // enqueue a call to the functions connected to the signal
  template<typename... ArgTypes>
  static inline bool enqueue(const signal<ArgTypes...>& sig, ArgTypes&... args) noexcept
  {
    if(!sig.empty()) // ensure that invalid signals are not enqueued
    {
      std::lock_guard<lockable<std::queue<vfunc>>> lock(Application::ms_signal_queue); // multithread protection
      for(auto sigpair : sig) // iterate through all connected slots
        Application::ms_signal_queue.emplace(std::bind(sigpair.second, sigpair.first, std::forward<ArgTypes>(args)...));
      Application::step(); // inform execution stepper
      return true;
    }
    return false;
  }

  // enqueue a call to the functions connected to the signal with /copies/ of the arguments
  template<typename... ArgTypes>
  static inline bool enqueue_copy(const signal<ArgTypes...>& sig, ArgTypes... args) noexcept
    { return enqueue(sig, args...);}
};

#endif // OBJECT_H
