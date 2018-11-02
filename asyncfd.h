#ifndef ASYNCFD_H
#define ASYNCFD_H

// PUT
#include <object.h>
#include <cxxutils/posix_helpers.h>

class AsyncFD : public Object
{
public:
  AsyncFD(posix::fd_t fd) noexcept;

  void read (      void* buffer, posix::size_t size) noexcept;
  void write(const void* buffer, posix::size_t size) noexcept;

  signal<posix::fd_t, void*, posix::size_t> readComplete;
  signal<posix::fd_t, void*, posix::size_t, posix::error_t> readError;

  signal<posix::fd_t, posix::size_t> writeComplete;
  signal<posix::fd_t, posix::size_t, posix::error_t> writeError;
private:
  void async_read (      void* buffer, posix::size_t size, posix::off_t offset, posix::size_t remaining) noexcept;
  void async_write(const void* buffer, posix::size_t size, posix::off_t offset, posix::size_t remaining) noexcept;
  posix::fd_t m_fd;
};

#endif // ASYNCFD_H
