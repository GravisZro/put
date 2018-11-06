#ifndef MUTEX_H
#define MUTEX_H

#include <specialized/osdetect.h>

#if defined(_POSIX_THREADS)
// POSIX
#include <pthread.h>
typedef pthread_mutex_t mutex_type;
#endif

namespace posix
{
  class mutex
  {
  public:
    mutex(void) noexcept;
    bool lock(void) noexcept;
    bool unlock(void) noexcept;
  private:
    mutex_type m_mutex;
  };

  template<typename T>
  struct lockable : T, posix::mutex
  {
    template<typename... ArgTypes>
    lockable(ArgTypes... args) noexcept
      : T(args...) { }
  };
}

#endif // MUTEX_H
