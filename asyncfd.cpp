#include "asyncfd.h"

// POSIX++
#include <cassert>

AsyncFD::AsyncFD(posix::fd_t fd) noexcept
  : m_fd(fd)
{
  assert(posix::donotblock(m_fd));
}

void AsyncFD::read(void* buffer, posix::size_t size) noexcept
  { async_read(buffer, size, 0, size); }

void AsyncFD::write(const void* buffer, posix::size_t size) noexcept
  { async_write(buffer, size, 0, size); }

void AsyncFD::async_read(void* buffer, posix::size_t size, posix::off_t offset, posix::size_t remaining) noexcept
{
  posix::ssize_t out = posix::ignore_interruption(::pread, m_fd, buffer, remaining, offset);
  if(out < 0)
  {
    size -= remaining;
    Object::enqueue(readError, m_fd, buffer, size, errno);
  }
  else
  {
    assert(remaining >= posix::size_t(out));
    offset += out;
    remaining -= posix::size_t(out);
    if(remaining == 0)
      Object::enqueue(readComplete, m_fd, buffer, size);
    else
      Object::singleShot(this, &AsyncFD::async_read, buffer, size, offset, remaining);
  }
}

void AsyncFD::async_write(const void* buffer, posix::size_t size, posix::off_t offset, posix::size_t remaining) noexcept
{
  posix::ssize_t out = posix::ignore_interruption(::pwrite, m_fd, buffer, remaining, offset);
  if(out < 0)
  {
    size -= remaining;
    Object::enqueue(writeError, m_fd, size, errno);
  }
  else
  {
    assert(remaining >= posix::size_t(out));
    offset += out;
    remaining -= posix::size_t(out);
    if(remaining == 0)
      Object::enqueue(writeComplete, m_fd, size);
    else
      Object::singleShot(this, &AsyncFD::async_write, buffer, size, offset, remaining);
  }
}
