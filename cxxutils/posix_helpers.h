#ifndef POSIX_HELPERS_H
#define POSIX_HELPERS_H

// STL
#include <string>
#include <cerrno>
#include <set>

// POSIX
#include <pwd.h>
#include <grp.h>

// PDTK
#include "error_helpers.h"

namespace posix
{
  typedef int fd_t;
  static const fd_t invalid_descriptor = -1;

#ifndef __clang__
  template<typename RType, typename... ArgTypes>
  using function = RType(*)(ArgTypes...);

  template<typename RType, typename... ArgTypes>
  static inline RType ignore_interruption(function<RType, ArgTypes...> func, ArgTypes... args)
#else
  template<typename RType, typename... ArgTypes>
  static inline RType ignore_interruption(RType(*func)(ArgTypes...), ArgTypes... args)
#endif
  {
    RType rval;
    do {
      rval = func(args...);
    } while(rval == error_response && errno == std::errc::interrupted);
    return rval;
  }

#ifndef __clang__
  template<typename RType, typename... ArgTypes>
  static inline RType* ignore_interruption(function<RType*, ArgTypes...> func, ArgTypes... args)
#else
  template<typename RType, typename... ArgTypes>
  static inline RType* ignore_interruption(RType*(*func)(ArgTypes...), ArgTypes... args)
#endif
  {
    RType* rval;
    do {
      rval = func(args...);
    } while(rval == nullptr && errno == std::errc::interrupted);
    return rval;
  }

// POSIX wrappers
  static inline passwd* getpwuid(uid_t uid)
    { return ignore_interruption<passwd, __uid_t>(::getpwuid, uid); }

  static inline group* getgrgid(gid_t gid)
    { return ignore_interruption<group, __gid_t>(::getgrgid, gid); }

// shortcuts
  static inline std::string getusername(uid_t uid)
  {
    passwd* rval = posix::getpwuid(uid);
    return rval == nullptr ? "" : rval->pw_name;
  }

  static inline std::string getgroupname(gid_t gid)
  {
    group* rval = posix::getgrgid(gid);
    return rval == nullptr ? "" : rval->gr_name;
  }
}

#endif // POSIX_HELPERS_H
