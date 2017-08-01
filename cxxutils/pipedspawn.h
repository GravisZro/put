#ifndef PIPEDSPAWN_H
#define PIPEDSPAWN_H

// PDTK
#include <cxxutils/vfifo.h>
#include <cxxutils/posix_helpers.h>
#include <cxxutils/error_helpers.h>
#include <cxxutils/colors.h>

// Realtime POSIX
#include <spawn.h>

#ifndef SPAWN_PROGRAM_NAME
#define SPAWN_PROGRAM_NAME "pldstub"
#endif

class PipedSpawn
{
  enum {
    Read = 0,
    Write = 1,
  };
public:
  PipedSpawn(void) noexcept
    : m_pid(0),
      m_stdout(posix::error_response),
      m_stderr(posix::error_response)
  {
    posix::fd_t stdin_pipe[2];
    posix::fd_t stdout_pipe[2];
    posix::fd_t stderr_pipe[2];
    posix_spawn_file_actions_t action;

    flaw(::pipe(stdin_pipe ) == posix::error_response ||
         ::pipe(stdout_pipe) == posix::error_response ||
         ::pipe(stderr_pipe) == posix::error_response,
         posix::critical, , std::exit(2), "An 'impossible' situation has occurred.")

    posix_spawn_file_actions_init(&action);

    posix_spawn_file_actions_addclose(&action, stdin_pipe [Write]);
    posix_spawn_file_actions_addclose(&action, stdout_pipe[Read ]);
    posix_spawn_file_actions_addclose(&action, stderr_pipe[Read ]);

    posix_spawn_file_actions_adddup2 (&action, stdin_pipe [Read ], STDIN_FILENO );
    posix_spawn_file_actions_adddup2 (&action, stdout_pipe[Write], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2 (&action, stderr_pipe[Write], STDERR_FILENO);

    posix_spawn_file_actions_addclose(&action, stdin_pipe [Read ]);
    posix_spawn_file_actions_addclose(&action, stdout_pipe[Write]);
    posix_spawn_file_actions_addclose(&action, stderr_pipe[Write]);

    m_stdin  = stdin_pipe [Write];
    m_stdout = stdout_pipe[Read ];
    m_stderr = stderr_pipe[Read ];

    const char* args[2] = { SPAWN_PROGRAM_NAME, nullptr };

    flaw(posix_spawnp(&m_pid, SPAWN_PROGRAM_NAME, &action, nullptr, static_cast<char* const*>(const_cast<char**>(args)), nullptr) != posix::success_response, posix::severe,,,
      "posix_spawnp failed with error: %s", std::strerror(errno))
  }

  ~PipedSpawn(void) noexcept
  {
    m_pid = 0;
    posix::close(m_stdin);
    posix::close(m_stdout);
    posix::close(m_stderr);
  }

  posix::fd_t getStdIn (void) const noexcept { return m_stdin ; }
  posix::fd_t getStdOut(void) const noexcept { return m_stdout; }
  posix::fd_t getStdErr(void) const noexcept { return m_stderr; }

  pid_t processId(void) const noexcept { return m_pid; }

  bool writeStdIn(vfifo& vq) const noexcept { return write(m_stdin, vq); }
  bool readStdOut(vfifo& vq) const noexcept { return read(m_stdout, vq); }
  bool readStdErr(vfifo& vq) const noexcept { return read(m_stderr, vq); }

  bool waitReadStdOut(int timeout) const noexcept { return waitFDEvent(m_stdout, POLLIN , timeout); }
  bool waitReadStdErr(int timeout) const noexcept { return waitFDEvent(m_stderr, POLLIN , timeout); }
  bool waitWrite     (int timeout) const noexcept { return waitFDEvent(m_stdin , POLLOUT, timeout); }

protected:
  bool waitFDEvent(posix::fd_t fd, int16_t event, int timeout) const noexcept
  {
    pollfd fds = { fd, event, 0 };
    return posix::ignore_interruption(::poll, &fds, nfds_t(1), timeout) == 1;
  }

  static bool write(posix::fd_t fd, vfifo& vq) noexcept
    { return !vq.empty() && posix::write(fd, vq.begin(), vq.size()) == posix::ssize_t(vq.size()); }

  static bool read(posix::fd_t fd, vfifo& vq) noexcept
  {
    posix::ssize_t sz = posix::read(fd, vq.data(), vq.capacity());
    return sz > 0 && vq.resize(sz);
  }

private:
  pid_t m_pid;
  posix::fd_t m_stdin;
  posix::fd_t m_stdout;
  posix::fd_t m_stderr;
};

#endif // PIPEDSPAWN_H
