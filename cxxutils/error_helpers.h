#ifndef ERROR_HELPERS_H
#define ERROR_HELPERS_H

// C++
#include <cerrno>
#include <cstdio>

// STL
#include <system_error>


#define flaw(condition, msg_prefix, exec, rvalue, ...) \
  if(condition) \
  { \
    dprintf(STDERR_FILENO, msg_prefix); \
    dprintf(STDERR_FILENO, ##__VA_ARGS__); \
    dprintf(STDERR_FILENO, "\n"); \
    exec; \
    return rvalue; \
  }

template<typename T>
constexpr bool operator ==(std::errc err, T err_num) noexcept
  { return *reinterpret_cast<T*>(&err) == err_num; }

template<typename T>
constexpr bool operator ==(T err_num, std::errc err) noexcept
  { return int(err) == err_num; }

namespace posix
{
  constexpr int success_response = 0;
  constexpr int error_response = -1;
  static inline int success(void) noexcept { return errno = success_response; }
  static inline int error(std::errc err) noexcept { errno = int(err); return error_response; }
}

#endif // ERROR_HELPERS_H

