#ifndef ERROR_HELPERS_H
#define ERROR_HELPERS_H

// STL
#include <system_error>

template<typename T>
constexpr bool operator ==(std::errc err, T err_num)
  { return *reinterpret_cast<T*>(&err) == err_num; }

template<typename T>
constexpr bool operator ==(T err_num, std::errc err)
  { return (int)err == err_num; }

namespace posix
{
  static const int success_response = 0;
  static const int error_response = -1;
  static int success(void) { return errno = 0; }
  static int error(std::errc err) { return errno = (int)err ? error_response : success_response; }
}

#endif // ERROR_HELPERS_H

