#include "mutex.h"

// PUT
# include <put/cxxutils/error_helpers.h>

#if defined(SINGLE_THREADED_APPLICATION)
#elif defined(__tru64__) && !defined(FORCE_POSIX_MUTEXES)
posix::mutex::mutex(void) noexcept
{
  posix::error_t err = ::tis_mutex_init(&m_mutex);
  if(err)
    errno = err;
}

posix::mutex::~mutex(void) noexcept
{
  posix::error_t err = ::tis_mutex_destroy(&m_mutex);
  if(err)
    errno = err;
}

bool posix::mutex::lock(void) noexcept
{
  posix::error_t err = ::tis_mutex_lock(&m_mutex);
  if(err)
    errno = err;
  return err == posix::success_response;
}

bool posix::mutex::unlock(void) noexcept
{
  posix::error_t err = ::tis_mutex_unlock(&m_mutex);
  if(err)
    errno = err;
  return err == posix::success_response;
}


#elif defined(__plan9__) && !defined(FORCE_POSIX_MUTEXES)
static int lock_init = []() { lockinit(); return 0; }

posix::mutex::mutex(void) noexcept { }
posix::mutex::~mutex(void) noexcept { }

bool posix::mutex::lock(void) noexcept
{
  ::lock(&m_mutex);
  return true;
}

bool posix::mutex::unlock(void) noexcept
{
  ::unlock(&m_mutex);
  return true;
}


#elif defined(_POSIX_THREADS)
# if defined(FORCE_POSIX_MUTEXES)
#  pragma message("Forcing use of POSIX mutexes.")
# endif

posix::mutex::mutex(void) noexcept
{
  posix::error_t err = ::pthread_mutex_init(&m_mutex, NULL);
  if(err)
    errno = err;
}

posix::mutex::~mutex(void) noexcept
{
  posix::error_t err = ::pthread_mutex_destroy(&m_mutex);
  if(err)
    errno = err;
}

bool posix::mutex::lock(void) noexcept
{
  posix::error_t err = ::pthread_mutex_lock(&m_mutex);
  if(err)
    errno = err;
  return err == posix::success_response;
}

bool posix::mutex::unlock(void) noexcept
{
  posix::error_t err = ::pthread_mutex_unlock(&m_mutex);
  if(err)
    errno = err;
  return err == posix::success_response;
}
#endif
