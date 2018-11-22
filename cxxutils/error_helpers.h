#ifndef ERROR_HELPERS_H
#define ERROR_HELPERS_H

// POSIX++
#include <cerrno>
#include <cstdio>

// STL
#include <system_error>


#define flaw(condition, msg_prefix, exec, rvalue, ...) \
  if(condition) \
  { \
    std::fprintf(stderr, msg_prefix); \
    std::fprintf(stderr, ##__VA_ARGS__); \
    std::fprintf(stderr, "\n"); \
    exec; \
    return rvalue; \
  }

template<typename T>
constexpr bool operator ==(std::errc err, T err_num) noexcept
  { return *reinterpret_cast<T*>(&err) == err_num; }

template<typename T>
constexpr bool operator ==(T err_num, std::errc err) noexcept
  { return T(err) == err_num; }

namespace posix
{
  typedef int error_t;
  constexpr error_t success_response = 0;
  constexpr error_t error_response = -1;
  static inline bool check_errno(void) { return errno == success_response; }
  static inline bool success(void) noexcept { errno = success_response; return true; }
  static inline bool error(error_t err) noexcept { errno = err; return check_errno(); }
  static inline bool error(std::errc err) noexcept { return error(error_t(err)); }
  template<typename T> static inline error_t errorval(T err) noexcept { return error(err) ? posix::success_response : posix::error_response; }
}

#endif // ERROR_HELPERS_H

