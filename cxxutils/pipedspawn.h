#ifndef PIPEDSPAWN_H
#define PIPEDSPAWN_H

// PUT
#include <put/cxxutils/posix_helpers.h>
#include <put/cxxutils/vterm.h>
#include <put/cxxutils/vfifo.h>

// Realtime POSIX
#include <spawn.h>

#ifndef SPAWN_PROGRAM_NAME
#define SPAWN_PROGRAM_NAME "executor"
#endif

class PipedSpawn
{
public:
  PipedSpawn(void) noexcept
    : m_pid(0),
      m_stdin (posix::invalid_descriptor),
      m_stdout(posix::invalid_descriptor),
      m_stderr(posix::invalid_descriptor)
  {
    enum {
      Read = 0,
      Write = 1,
    };
    posix::fd_t stdin_pipe[2];
    posix::fd_t stdout_pipe[2];
    posix::fd_t stderr_pipe[2];
    posix_spawn_file_actions_t action;

    flaw(!posix::pipe(stdin_pipe ) ||
         !posix::pipe(stdout_pipe) ||
         !posix::pipe(stderr_pipe),
         terminal::critical,,,
         "Unable to create a pipe: %s", posix::strerror(errno))

    posix::fcntl(stdin_pipe[Read ], F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::fcntl(stdin_pipe[Write], F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::fcntl(stdout_pipe[Read ], F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::fcntl(stdout_pipe[Write], F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::fcntl(stderr_pipe[Read ], F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::fcntl(stderr_pipe[Write], F_SETFD, FD_CLOEXEC); // close on exec*()

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

    flaw(posix_spawnp(&m_pid,
                      SPAWN_PROGRAM_NAME, &action, NULL,
                      static_cast<char* const*>(const_cast<char**>(args)), NULL) != posix::success_response,
         terminal::severe,,,
         "posix_spawnp failed with error: %s", posix::strerror(errno))
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

protected:
  static bool write(posix::fd_t fd, vfifo& vq) noexcept
    { return !vq.empty() && posix::write(fd, vq.begin(), posix::size_t(vq.size())) == vq.size(); }

  static bool read(posix::fd_t fd, vfifo& vq) noexcept
  {
    posix::ssize_t sz = posix::read(fd, vq.data(), posix::size_t(vq.capacity()));
    return sz > 0 && vq.resize(sz);
  }

private:
  pid_t m_pid;
  posix::fd_t m_stdin;
  posix::fd_t m_stdout;
  posix::fd_t m_stderr;
};

#endif // PIPEDSPAWN_H
