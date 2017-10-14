#include "PollEvent.h"

#include <functional>

PollEvent::PollEvent(posix::fd_t _fd, Event::Flags _flags) noexcept
  : m_fd(_fd), m_flags(_flags)
{
  EventBackend::add(m_fd, m_flags, // connect FD with flags to signal
                    [this](posix::fd_t lambda_fd, Event::Flags_t lambda_flags) noexcept
                    { Object::enqueue(activated, lambda_fd, lambda_flags); });
}

PollEvent::~PollEvent(void) noexcept
{
  EventBackend::remove(m_fd, m_flags); // disconnect FD with flags from signal
}
