#include "mutex.h"

// PUT
#include "error_helpers.h"

posix::mutex::mutex(void) noexcept
 : m_mutex(PTHREAD_MUTEX_INITIALIZER)
{
}

bool posix::mutex::lock(void) noexcept
{
  posix::error_t err = pthread_mutex_lock(&m_mutex);
  if(err)
    errno = err;
  return err == posix::success_response;
}

bool posix::mutex::unlock(void) noexcept
{
  posix::error_t err = pthread_mutex_unlock(&m_mutex);
  if(err)
    errno = err;
  return err == posix::success_response;
}
