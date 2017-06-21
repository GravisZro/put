#include "process.h"

// POSIX
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>

// C++
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cstring>

// PDTK
#include <specialized/procstat.h>
#include <specialized/eventbackend.h>
#include <cxxutils/cstringarray.h>

enum class command : uint8_t
{
  invoke = 0,
  executable,
  arguments,
  environment,
  environmentvar,
  workingdir,
  priority,
  uid,
  gid,
  euid,
  egid,
  resource
};

static inline vfifo& operator << (vfifo& vq, command cmd) noexcept
{ return vq << static_cast<uint8_t>(cmd); }

static std::unordered_map<pid_t, Process*> process_map; // do not try to own Process memory

void Process::init_once(void) noexcept
{
  static bool ok = true;
  if(ok)
  {
    ok = false;
    struct sigaction actions;
    actions.sa_handler = &reaper;
    ::sigemptyset(&actions.sa_mask);
    actions.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if(::sigaction(SIGCHLD, &actions, nullptr) == posix::error_response)
    {
      ::perror("An 'impossible' situation has occurred.");
      ::exit(1);
    }
  }
}

void Process::reaper(int sig) noexcept
{
  assert(sig == SIGCHLD); // there should only be one way to get here, so be sure there is no funny business
  pid_t pid = posix::error_response; // set value just in case
  int status = 0;
  while((pid = ::waitpid(pid_t(-1), &status, WNOHANG)) != posix::error_response) // get the next dead process (if there is one)... while the currently reaped process was valid
  {
    auto process_map_iter = process_map.find(pid); // find dead process
    if(process_map_iter != process_map.end()) // if the dead process exists...
    {
      Process* p = process_map_iter->second;
      EventBackend::remove(p->getStdOut());
      EventBackend::remove(p->getStdErr());
      ::close(p->getStdOut());
      ::close(p->getStdErr());
      ::close(p->getStdIn());
      p->m_state = Process::State::Finished;
      process_map.erase(process_map_iter); // remove finished process from the process map
    }
  }
}

Process::Process(void) noexcept
  : m_state(State::Initializing),
    m_error(Error::None)
{
  init_once();
  process_map.emplace(processId(), this); // add self to process map
  Object::connect(EventBackend::watch(processId(), EventFlags::ExecEvent), started);
  Object::connect(EventBackend::watch(processId(), EventFlags::ExitEvent), finished);
}

Process::~Process(void) noexcept
{
  if(m_state == State::Running)
    sendSignal(posix::signal::Kill);

  m_state = State::Invalid;
}

bool Process::write_then_read(void) noexcept
{
  if(m_iobuf.empty() || // ensure there is data to write
     !PipedSpawn::writeStdIn(m_iobuf) || // write data to pipe
     !PipedSpawn::waitReadStdOut(1000) || // wait to read pipe
     !PipedSpawn::readStdOut(m_iobuf) || // read data from pipe
     (m_iobuf >> errno).hadError()) // read return value
    return false;
  return errno == posix::success_response;
}

bool Process::setArguments(const std::vector<std::string>& arguments) noexcept
{
  m_iobuf.reset();
  m_iobuf << command::arguments;
  for(const std::string& arg : arguments)
    m_iobuf << arg;
  return write_then_read();
}

bool Process::setEnvironment(const std::unordered_map<std::string, std::string>& environment) noexcept
{
  m_iobuf.reset();
  m_iobuf << command::environment;
  for(const std::pair<std::string, std::string>& p : environment)
    m_iobuf << p.first << p.second;
  return write_then_read();
}

bool Process::setEnvironmentVariable(const std::string& name, const std::string& value) noexcept
{
  m_iobuf.reset();
  m_iobuf << command::environment << name << value;
  return write_then_read();
}

bool Process::setResourceLimit(Process::Resource which, rlim_t limit) noexcept
{
  m_iobuf.reset();
  m_iobuf << command::resource
          << static_cast<int>(which)
          << static_cast<rlim_t>(RLIM_SAVED_CUR)
          << limit;
  return write_then_read();
}

bool Process::setWorkingDirectory(const std::string& dir) noexcept
{
  m_iobuf.reset();
  m_iobuf << command::workingdir << dir;
  return write_then_read();
}

bool Process::setExecutable(const std::string& executable) noexcept
{
  m_iobuf.reset();
  m_iobuf << command::executable << executable;
  return write_then_read();
}

bool Process::setUserID(uid_t id) noexcept
{
  if(posix::getpwuid(id) == nullptr)
    return false;

  m_iobuf.reset();
  m_iobuf << command::uid << id;
  return write_then_read();
}

bool Process::setGroupID(gid_t id) noexcept
{
  if(posix::getgrgid(id) == nullptr)
    return false;

  m_iobuf.reset();
  m_iobuf << command::gid << id;
  return write_then_read();
}

bool Process::setEffectiveUserID(uid_t id) noexcept
{
  if(posix::getpwuid(id) == nullptr)
    return false;

  m_iobuf.reset();
  m_iobuf << command::euid << id;
  return write_then_read();
}

bool Process::setEffectiveGroupID(gid_t id) noexcept
{
  if(posix::getgrgid(id) == nullptr)
    return false;

  m_iobuf.reset();
  m_iobuf << command::egid << id;
  return write_then_read();
}

bool Process::setPriority(int nval) noexcept
{
#if defined(PRIO_MIN) && defined(PRIO_MAX)
  if(nval < PRIO_MIN || nval > PRIO_MAX)
    return false;
#else
#warning PRIO_MIN or PRIO_MAX is not defined.  The safegaurd in Process::setPriority() is disabled.
#endif

  m_iobuf.reset();
  m_iobuf << command::priority << nval;
  return write_then_read();
}

bool Process::sendSignal(posix::signal::EId id, int value) const noexcept
{
  return posix::signal::send(PipedSpawn::processId(), id, value);
}

bool Process::invoke(void) noexcept
{
  assert(m_state == State::Initializing);

  m_iobuf.reset();
  m_iobuf << command::invoke;

  if(!PipedSpawn::writeStdIn(m_iobuf))
    return false;

  process_state_t data;
  if(!::procstat(PipedSpawn::processId(), data))
  {
    m_state = State::Invalid;
    return false;
  }

  Object::connect(EventBackend::watch(getStdOut()), stdoutMessage);
  Object::connect(EventBackend::watch(getStdOut()), stderrMessage);

  m_state = State::Running;
  return errno == posix::success_response;
}

Process::State Process::state(void) noexcept
{
  switch(m_state)
  {
    case State::Finished:
    case State::Initializing:
      break;
    default:
      process_state_t data;
      assert(::procstat(PipedSpawn::processId(), data)); // process _must_ exist
      switch (data.state)
      {
        case WaitingInterruptable:
        case WaitingUninterruptable:
          m_state = State::Waiting; break;
        case Zombie : m_state = State::Zombie ; break;
        case Stopped: m_state = State::Stopped; break;
        case Running: m_state = State::Running; break;
      }
  }
  return m_state;
}
