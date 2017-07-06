#ifndef PIPEDFORK_H
#define PIPEDFORK_H

// PDTK
#include <cxxutils/posix_helpers.h>
#include <cxxutils/vfifo.h>

// POSIX
#include <poll.h>

class PipedFork
{
  enum {
    Read = 0,
    Write = 1,
  };
public:
  PipedFork(void) noexcept
    : m_pid(0),
      m_stdout(posix::error_response),
      m_stderr(posix::error_response)
  {

    posix::fd_t ipc_pico[2]; // Parent In Child Out
    posix::fd_t ipc_cipo[2]; // Child In Parent Out
    posix::fd_t out[2];
    posix::fd_t err[2];

    if(::pipe(ipc_pico) == posix::error_response ||
       ::pipe(ipc_cipo) == posix::error_response ||
       ::pipe(out) == posix::error_response ||
       ::pipe(err) == posix::error_response ||
       (m_pid = ::fork()) <= posix::error_response)
    {
      assert(false);
    }
    else if(m_pid) // if parent process
    {
      m_read = ipc_cipo[Read];
      posix::close(ipc_cipo[Write]);

      m_write = ipc_pico[Write];
      posix::close(ipc_pico[Read]);

      m_stdout = out[Read]; // copy open pipes
      posix::close(out[Write]); // ok if these fail

      m_stderr = err[Read];
      posix::close(err[Write]);
    }
    else // if child process
    {
      m_read = ipc_pico[Read];
      posix::close(ipc_pico[Write]);

      m_write = ipc_cipo[Write];
      posix::close(ipc_cipo[Read]);

      redirect(err[Write], STDERR_FILENO); // redirect stderr to interprocess pipe
      posix::close(err[Read]);

      redirect(out[Write], STDOUT_FILENO); // redirect stdout to interprocess pipe
      posix::close(out[Read]);
    }
  }

  ~PipedFork(void) noexcept
  {
    m_pid = 0;
    posix::close(m_read);
    posix::close(m_write);
    posix::close(m_stdout);
    posix::close(m_stderr);
  }

  posix::fd_t getStdOut(void) const noexcept { return m_stdout; }
  posix::fd_t getStdErr(void) const noexcept { return m_stderr; }

  pid_t id(void) const noexcept { return m_pid; }

  bool isChildProcess(void) const noexcept { return !m_pid; }

  bool readStdOut(vfifo& vq) noexcept { return read(m_stdout, vq); }
  bool readStdErr(vfifo& vq) noexcept { return read(m_stderr, vq); }

  bool read (vfifo& vq) noexcept { return read (m_read , vq); }
  bool write(vfifo& vq) noexcept { return write(m_write, vq); }

  bool waitReadStdOut(int timeout) const noexcept { return waitFDEvent(m_stdout, POLLIN , timeout); }
  bool waitReadStdErr(int timeout) const noexcept { return waitFDEvent(m_stderr, POLLIN , timeout); }
  bool waitRead      (int timeout) const noexcept { return waitFDEvent(m_read  , POLLIN , timeout); }
  bool waitWrite     (int timeout) const noexcept { return waitFDEvent(m_write , POLLOUT, timeout); }

protected:
  bool waitFDEvent(posix::fd_t fd, int16_t event, int timeout) const noexcept
  {
    pollfd fds = { fd, event, 0 };
    return posix::ignore_interruption(::poll, &fds, nfds_t(1), timeout) == 1;
  }

  bool redirect(posix::fd_t replacement, posix::fd_t original) const noexcept
    { return posix::dup2(replacement, original) && posix::close(replacement); }

  bool write(posix::fd_t fd, vfifo& vq) const noexcept
    { return !vq.empty() && posix::write(fd, vq.begin(), vq.size()) == vq.size(); }

  bool read(posix::fd_t fd, vfifo& vq) const noexcept
  {
    volatile posix::ssize_t sz = posix::read(fd, vq.data(), vq.capacity());
    return sz > 0 && vq.resize(sz);
  }

  posix::fd_t m_read;
  posix::fd_t m_write;

private:
  pid_t m_pid;
  posix::fd_t m_stdout;
  posix::fd_t m_stderr;
};

#endif // PIPEDFORK_H
