#include "process.h"

// POSIX
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>

// STL
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
//#include <cstring>

// PDTK
#include "cxxutils/cstringarray.h"

#define assertE(condition) \
  if(!(condition)) { std::perror("Error"); } \
  assert(condition);

Process::Process(void)
  : m_state(NotStarted),
    m_pid(0),
    m_uid(0),
    m_gid(0),
    m_stdout(posix::error_response),
    m_stderr(posix::error_response)
{
}

Process::~Process(void)
{
  m_state = NotStarted;
  m_pid = 0;
  m_uid = 0;
  m_gid = 0;

  if(m_stdout != posix::error_response)
    posix::close(m_stdout);
  if(m_stderr != posix::error_response)
    posix::close(m_stderr);
}

bool Process::setWorkingDirectory(const std::string& dir)
{
  struct stat statbuf;
  if(::stat(dir.c_str(), &statbuf) == posix::success_response)
    m_workingdir = dir;
  return errno == posix::success_response;
}

bool Process::setExecutable(const std::string& executable)
{
  struct stat statbuf;
  if(::stat(executable.c_str(), &statbuf) == posix::success_response)
    m_executable = executable;
  return errno == posix::success_response;
}

bool Process::start(void)
{
  if(m_executable.empty())
    return false;

  m_arguments.insert(m_arguments.begin(), m_executable);
  CStringArray argv(m_arguments);
  CStringArray envv(m_environment, [](const std::pair<std::string, std::string>& p) { return p.first + '=' + p.second; });

  posix::fd_t pipe_stdout[2];
  posix::fd_t pipe_stderr[2];

  if(::pipe(pipe_stdout) == posix::error_response ||
     ::pipe(pipe_stderr) == posix::error_response ||
     (m_pid = ::fork()) <= posix::error_response)
  {
    Object::enqueue_copy(error, UnknownError, static_cast<std::errc>(errno));
    return false;
  }


  if(m_pid == posix::success_response) // if inside forked process
  {
    // asserts will make it known where it failed via stderr
    assertE(posix::dup2(pipe_stdout[1], STDOUT_FILENO));
    assertE(posix::dup2(pipe_stderr[1], STDERR_FILENO));
    assertE(posix::close(pipe_stdout[0]));
    assertE(posix::close(pipe_stdout[1]));
    assertE(posix::close(pipe_stderr[0]));
    assertE(posix::close(pipe_stderr[1]));
    if(m_uid)
      assertE(::setreuid(m_uid, m_uid) == posix::success_response);
    if(m_gid)
      assertE(::setregid(m_gid, m_gid) == posix::success_response);
    assertE(::execve(argv[0], argv, envv) == posix::success_response);
    std::perror("Unreachable area has been reached!");
    assert(false); // unreachable area: _should_ be impossible
  }

  m_stdout = pipe_stdout[0];
  m_stderr = pipe_stderr[0];
  m_state = Running;

  if(!posix::close(pipe_stdout[1]) ||
     !posix::close(pipe_stderr[1]))
  {
    Object::enqueue_copy(error, UnknownError, static_cast<std::errc>(errno));
    return false;
  }

  if(m_priority.isValid() &&
     ::setpriority(PRIO_PROCESS, m_pid, m_priority) != posix::success_response)
  {
    Object::enqueue_copy(error, UnknownError, static_cast<std::errc>(errno));
    return false;
  }

  return true;
}

bool Process::sendSignal(posix::signal::EId id) const
{
  if(m_pid)
    return posix::signal::send(m_pid, id);
  return false;
}
