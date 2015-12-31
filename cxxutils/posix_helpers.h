#ifndef POSIX_HELPERS_H
#define POSIX_HELPERS_H

// STL
#include <string>
#include <cerrno>

// POSIX
#include <pwd.h>
#include <grp.h>

// PDTK
#include "error_helpers.h"

namespace posix
{
  typedef int fd_t;
  static const fd_t invalid_descriptor = -1;

  /*
    struct fdset_t : fd_set
    {
      inline ~fdset_t(void) { clear(); }

      inline operator fd_set*(void) { return this; }

      inline bool contains(fd_t fd) const { return FD_ISSET (fd, this); }
      inline void clear   (void   ) { FD_ZERO(    this); }
      inline void add     (fd_t fd) { FD_SET (fd, this); }
      inline void remove  (fd_t fd) { FD_CLR (fd, this); }
    };

    struct socket_t : fdset_t
    {
      socket_t(fd_t s) : m_socket(s) { }
      inline operator fd_t(void) { return m_socket; }
      inline fd_t operator =(fd_t s) { return m_socket = s; }

      fd_t m_socket;
    };
  */

#ifndef __clang__
  template<typename RType, typename... ArgTypes>
  using function = RType(*)(ArgTypes...);

  template<typename RType, typename... ArgTypes>
  static inline RType ignore_interruption(const RType bad_value, function<RType, ArgTypes...> func, ArgTypes... args)
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
    { return ignore_interruption(::getpwuid, uid); }

  static inline group* getgrgid(gid_t gid)
    { return ignore_interruption(::getgrgid, gid); }

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
