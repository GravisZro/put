#ifndef MUTEX_H
#define MUTEX_H

// POSIX
#include <pthread.h>

namespace posix
{
  class mutex
  {
  public:
    mutex(void) noexcept;
    bool lock(void) noexcept;
    bool unlock(void) noexcept;
  private:
    pthread_mutex_t m_mutex;
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
