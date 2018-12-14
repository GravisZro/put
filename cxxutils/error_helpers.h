#ifndef ERROR_HELPERS_H
#define ERROR_HELPERS_H

// POSIX
#include <errno.h>
#include <stdio.h>

// STL
#include <system_error>


#define flaw(condition, msg_prefix, exec, rvalue, ...) \
  if(condition) \
  { \
    posix::fprintf(stderr, msg_prefix); \
    posix::fprintf(stderr, ##__VA_ARGS__); \
    posix::fprintf(stderr, "\n"); \
    exec; \
    return rvalue; \
  }

namespace posix
{
  using std::errc;
  typedef int error_t;
  constexpr error_t success_response = 0;
  constexpr error_t error_response = -1;
  static inline bool is_success(void) { return errno == success_response; }
  static inline bool success(void) noexcept { errno = success_response; return true; }
  static inline bool error(error_t err) noexcept { errno = err; return is_success(); }
  static inline bool error(errc err) noexcept { return error(error_t(err)); }
  template<typename T> static inline error_t errorval(T err) noexcept { return error(err) ? posix::success_response : posix::error_response; }

  template<typename T>
  constexpr bool operator ==(errc err, T err_num) noexcept
    { return *reinterpret_cast<T*>(&err) == err_num; }

  template<typename T>
  constexpr bool operator !=(errc err, T err_num) noexcept
    { return *reinterpret_cast<T*>(&err) != err_num; }

  template<typename T>
  constexpr bool operator ==(T err_num, errc err) noexcept
    { return T(err) == err_num; }

  template<typename T>
  constexpr bool operator !=(T err_num, errc err) noexcept
    { return T(err) != err_num; }
}

#endif // ERROR_HELPERS_H

