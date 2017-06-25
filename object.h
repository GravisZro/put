#ifndef OBJECT_H
#define OBJECT_H

// PDTK
#include <cxxutils/posix_helpers.h>
#include <application.h>
#include <specialized/eventbackend.h>

// STL
#include <functional>
#include <list>
#include <iostream>
struct ProtoObject
{
  inline  ProtoObject(void) noexcept { self = this; }
  inline ~ProtoObject(void) noexcept { self = nullptr; }
  ProtoObject* self; // used to determine if type has been deleted
};

class Object : private ProtoObject
{
public:
  template<typename... ArgTypes>
  using signal = std::list<std::pair<ProtoObject*, std::function<void(ProtoObject*, ArgTypes...)>>>;

  inline Object(void) noexcept  { }
  inline ~Object(void) noexcept { }

  // connect to a member of an object
  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, RType(ObjType::*slot)(ArgTypes...)) noexcept
  {
    sig.emplace_back(std::make_pair(static_cast<ProtoObject*>(obj),
      [slot](ProtoObject* p, ArgTypes... args) noexcept
        { if(p == p->self) (static_cast<ObjType*>(p)->*slot)(args...); })); // if ProtoObject is valid (not deleted), call slot
  }

  // connect to another signal
  template<typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig1, signal<ArgTypes...>& sig2) noexcept
  {
    sig1.emplace_back(std::make_pair(static_cast<ProtoObject*>(nullptr),
      [&sig2](ProtoObject*, ArgTypes... args) noexcept
        { enqueue(sig2, args...); }));
  }

  // connect to a function that accept the object pointer as the first argument
  template<class ObjType, typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, ObjType* obj, RType(*slot)(ObjType*, ArgTypes...)) noexcept
  {
    sig.emplace_back(std::make_pair(static_cast<ProtoObject*>(obj),
      [slot](ProtoObject* p, ArgTypes... args) noexcept
        { if(p == p->self) slot(static_cast<ObjType*>(p), args...); })); // if ProtoObject is valid (not deleted), call slot
  }

  // connect to a function and ignore the object
  template<typename RType, typename... ArgTypes>
  static inline void connect(signal<ArgTypes...>& sig, RType(*slot)(ArgTypes...)) noexcept
  {
    sig.emplace_back(std::make_pair(static_cast<ProtoObject*>(nullptr),
      [slot](ProtoObject*, ArgTypes... args) noexcept
        { slot(args...); }));
  }


  // connect a file descriptor event to an object member function
  template<class ObjType, typename RType>
  static inline void connect(posix::fd_t fd, EventFlags_t flags, ObjType* obj, RType(ObjType::*slot)(posix::fd_t, EventData_t)) noexcept
  {
    Application::ms_fd_signals.emplace(std::make_pair(fd, std::make_pair(flags,
      [obj, slot](posix::fd_t _fd, EventData_t data) noexcept
        { if(obj == obj->self) (obj->*slot)(_fd, data); }))); // if ProtoObject is valid (not deleted), call slot
  }

  template<class ObjType, typename RType>
  static inline void connect(posix::fd_t fd, ObjType* obj, RType(ObjType::*slot)(posix::fd_t, EventData_t)) noexcept
    { connect(fd, EventFlags::Readable, obj, slot); }

  // connect an file descriptor event to a signal
  static inline void connect(posix::fd_t fd, EventFlags_t flags, signal<posix::fd_t, EventData_t>& sig) noexcept
  {
    Application::ms_fd_signals.emplace(std::make_pair(fd, std::make_pair(flags,
      [&sig](posix::fd_t _fd, EventData_t _data) noexcept
      {
        if(!sig.empty()) // ensure that invalid signals are not enqueued
        {
          std::lock_guard<lockable<std::queue<vfunc>>> lock(Application::ms_signal_queue); // multithread protection
          for(auto sigpair : sig) // iterate through all connected slots
            Application::ms_signal_queue.emplace(std::bind(sigpair.second, sigpair.first, _fd, _data));
          Application::step(); // inform execution stepper
        }
      })));
  }

  static inline void connect(posix::fd_t fd, signal<posix::fd_t, EventData_t>& sig) noexcept
    { connect(fd, EventFlags::Readable, sig); }


  // connect a file descriptor event to a function
  template<typename RType>
  static inline void connect(posix::fd_t fd, EventFlags_t flags, RType(*slot)(posix::fd_t, EventData_t)) noexcept
  {
    Application::ms_fd_signals.emplace(std::make_pair(fd, std::make_pair(flags,
      [slot](posix::fd_t fd, EventData_t data) noexcept
        { slot(fd, data); })));
  }

  template<typename RType>
  static inline void connect(posix::fd_t fd, RType(*slot)(posix::fd_t, EventData_t)) noexcept
    { connect(fd, EventFlags::Readable, slot); }


  // enqueue a call to the functions connected to the signal
  template<typename... ArgTypes>
  static inline bool enqueue(signal<ArgTypes...>& sig, ArgTypes&... args) noexcept
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
  static inline bool enqueue_copy(signal<ArgTypes...>& sig, ArgTypes... args) noexcept
    { return enqueue(sig, args...);}
};

#endif // OBJECT_H
