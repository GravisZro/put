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
  static inline error_t success(void) noexcept { return errno = success_response; }
  static inline error_t error(error_t err) noexcept { errno = err; return error_response; }
  static inline error_t error(std::errc err) noexcept { return error(error_t(err)); }
}

#endif // ERROR_HELPERS_H

