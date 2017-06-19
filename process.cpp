#include "process.h"

// POSIX
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

// for debug
#include <iostream>

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

Process::Process(void) noexcept
  : m_state(State::Initializing),
    m_error(Error::None)
{
  Object::connect(EventBackend::watch(processId(), EventFlags::ExitEvent), finished);
  Object::connect(EventBackend::watch(processId(), EventFlags::ExecEvent), started);
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
  if(m_state != State::Finished) // if the process hasn't been reaped already
  {
    process_state_t data;
    if(!::procstat(PipedSpawn::processId(), data)) // check if running
      m_state = State::Invalid; // if not running
    else if(m_state != State::Initializing) // if not in the process loader stub
    {
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
  }
  return m_state;
}
