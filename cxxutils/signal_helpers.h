#ifndef SIGNAL_HELPERS_H
#define SIGNAL_HELPERS_H

// POSIX++
#include <csignal>

// PUT
#include "error_helpers.h"

#ifndef SIGPOLL
# ifdef SIGIO
#  define SIGPOLL SIGIO
# else
#  error Neither SIGPOLL nor SIGIO is defined
# endif
#endif

#if defined(SA_RESTART) && !defined(INTERRUPTED_WRAPPER)
#pragma message("All interrupt handlers need to use the SA_RESTART flag!")
#endif

namespace posix
{
  template<typename RType, typename... ArgTypes>
  using function = RType(*)(ArgTypes...);

#if defined(SA_RESTART) && !defined(INTERRUPTED_WRAPPER)
  template<typename RType, typename... ArgTypes> constexpr RType  ignore_interruption(function<RType , ArgTypes...> func, ArgTypes... args) noexcept { return func(args...); }
  template<typename RType, typename... ArgTypes> constexpr RType* ignore_interruption(function<RType*, ArgTypes...> func, ArgTypes... args) noexcept { return func(args...); }
#else
  template<typename RType, typename... ArgTypes>
  static inline RType ignore_interruption(function<RType, ArgTypes...> func, ArgTypes... args) noexcept
  {
    RType rval = error_response;
    do {
      rval = func(args...);
    } while(rval == error_response && errno == std::errc::interrupted);
    return rval;
  }

  template<typename RType, typename... ArgTypes>
  static inline RType* ignore_interruption(function<RType*, ArgTypes...> func, ArgTypes... args) noexcept
  {
    RType* rval = NULL;
    do {
      rval = func(args...);
    } while(rval == NULL && errno == std::errc::interrupted);
    return rval;
  }
#endif
}
#endif // SIGNAL_HELPERS_H
