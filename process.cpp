#include "process.h"

// POSIX
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>

// C++
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <climits>

// PDTK
#include <cxxutils/cstringarray.h>

#define ATTN(type) std::fprintf(stdout, "ATTN: %s\n", type)
#define ABORT_ERROR(type, ...) \
  { \
    std::fprintf(stderr, "ERROR: %s\nfile: %s\nline: %d\n", type, __FILE__, __LINE__); \
    std::fprintf(stderr, __VA_ARGS__);  \
    ::abort(); \
  }

#define ASSERT_ERROR(expr) \
  { \
    if(!(expr)) \
      ABORT_ERROR("ABORTED", "expression: %s\nerrno: %d\nmessage: %s\n", #expr, errno, std::strerror(errno)); \
  }


Process::Process(void) noexcept
  : m_state(State::Invalid),
    m_error(Error::Unknown),
    m_pid (0),
    m_uid (0),
    m_gid (0),
    m_euid(0),
    m_egid(0),
    m_priority(INT_MAX),
    m_stdout(posix::error_response),
    m_stderr(posix::error_response)
{
  m_arguments.reserve(256);
}

Process::~Process(void) noexcept
{
  if(m_state == State::Running)
    sendSignal(posix::signal::Kill);

  m_state = State::Invalid;
  m_pid  = 0;
  m_uid  = 0;
  m_gid  = 0;
  m_euid = 0;
  m_egid = 0;

  if(m_stdout != posix::error_response)
    posix::close(m_stdout);
  if(m_stderr != posix::error_response)
    posix::close(m_stderr);
}

bool Process::setWorkingDirectory(const std::string& dir) noexcept
{
  struct stat statbuf;
  if(::stat(dir.c_str(), &statbuf) == posix::success_response)
    m_workingdir = dir;
  return errno == posix::success_response;
}

bool Process::setExecutable(const std::string& executable) noexcept
{
  struct stat statbuf;
  if(::stat(executable.c_str(), &statbuf) == posix::success_response)
  {
    if(m_executable.empty())
      m_arguments.insert(m_arguments.begin(), executable);
    else
      m_arguments.front() = executable;
    m_executable = executable;
    m_state = State::Defined;
  }
  return errno == posix::success_response;
}

void Process::setArguments(const std::vector<std::string>& arguments) noexcept
{
  m_arguments = arguments;
  if(!m_executable.empty())
    m_arguments.insert(m_arguments.begin(), m_executable);
}

bool Process::setUserID(uid_t id) noexcept
{
  if(posix::getpwuid(id) == nullptr)
    return false;
  m_uid = id;
  return true;
}

bool Process::setGroupID(gid_t id) noexcept
{
  if(posix::getgrgid(id) == nullptr)
    return false;
  m_gid = id;
  return true;
}

bool Process::setEffectiveUserID(uid_t id) noexcept
{
  if(posix::getpwuid(id) == nullptr)
    return false;
  m_euid = id;
  return true;
}

bool Process::setEffectiveGroupID(gid_t id) noexcept
{
  if(posix::getgrgid(id) == nullptr)
    return false;
  m_egid = id;
  return true;
}

bool Process::setPriority(int nval) noexcept
{
#if defined(PRIO_MIN) && defined(PRIO_MAX)
  if(nval < PRIO_MIN || nval > PRIO_MAX)
    return false;
#else
#warning PRIO_MIN or PRIO_MAX is not defined.  The safegaurd in Process::setPriority() is disabled.
#endif
  m_priority = nval;
  return true;
}

bool Process::sendSignal(posix::signal::EId id, int value) const noexcept
{
  if(m_pid)
    return posix::signal::send(m_pid, id, value);
  return false;
}

bool Process::start(void) noexcept
{
  if(m_state == State::Invalid)
    return false;

  CStringArray argv(m_arguments);
  CStringArray envv(m_environment, [](const std::pair<std::string, std::string>& p) { return p.first + '=' + p.second; });

  posix::fd_t pipe_stdout[2];
  posix::fd_t pipe_stderr[2];

  if(::pipe(pipe_stdout) == posix::error_response ||
     ::pipe(pipe_stderr) == posix::error_response ||
     (m_pid = ::fork()) <= posix::error_response)
  {
    m_error = Error::FailedToStart;
    Object::enqueue_copy(error, m_error, static_cast<std::errc>(errno));
    return false;
  }

  m_state = State::Loading;

  if(m_pid == posix::success_response) // if inside forked process
  {
    // asserts will make it known where it failed via stderr
    ASSERT_ERROR(posix::dup2(pipe_stderr[1], STDERR_FILENO)); // redirect stderr to interprocess pipe
    ASSERT_ERROR(posix::dup2(pipe_stdout[1], STDOUT_FILENO)); // redirect stdout to interprocess pipe

    ASSERT_ERROR(posix::close(pipe_stdout[1])); // close former interprocess pipe
    ASSERT_ERROR(posix::close(pipe_stderr[1]));

    ASSERT_ERROR(posix::close(pipe_stdout[0])); // close non-forked side (unused)
    ASSERT_ERROR(posix::close(pipe_stderr[0]));

    ATTN("SETTINGS");

    if(m_priority != INT_MAX) // only if m_priority has been set
      ASSERT_ERROR(::setpriority(PRIO_PROCESS, getpid(), m_priority) != posix::success_response); // set priority
    for(auto& limit : m_limits)
      ASSERT_ERROR(::setrlimit(limit.first, &limit.second) != posix::success_response);
    if(m_uid)
      ASSERT_ERROR(::setuid(m_uid) == posix::success_response);
    if(m_gid)
      ASSERT_ERROR(::setgid(m_gid) == posix::success_response);
    if(m_euid)
      ASSERT_ERROR(::seteuid(m_euid) == posix::success_response);
    if(m_egid)
      ASSERT_ERROR(::setegid(m_egid) == posix::success_response);

    ATTN("LOADING");
    ASSERT_ERROR(::execve(argv[0], argv, envv) == posix::success_response);
    ABORT_ERROR("DANGER", "message: execve() implemenation error! This area should be unreachable!");
  }

  m_stdout = pipe_stdout[0]; // copy open pipes
  m_stderr = pipe_stderr[0];
  m_state = State::Running;



  // close forked side (unused) of pipe
  posix::close(pipe_stdout[1]); // ok if these fail
  posix::close(pipe_stderr[1]);

  //Object::enqueue(started);
  return true;
}
