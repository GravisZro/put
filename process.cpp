#include "process.h"

// POSIX
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

// POSIX++
#include <cerrno>
#include <cstdlib>
#include <climits>
#include <cstring>

// PDTK
#include <specialized/procstat.h>
#include <specialized/eventbackend.h>
#include <cxxutils/error_helpers.h>
#include <cxxutils/colors.h>


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
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    flaw(::sigaction(SIGCHLD, &actions, nullptr) == posix::error_response,
         posix::critical, std::exit(errno), ,
         "Unable assign action to a signal: %s", std::strerror(errno))
  }
}

void Process::reaper(int sig) noexcept
{
  flaw(sig != SIGCHLD, posix::warning, posix::error(std::errc::invalid_argument),,
       "Process::reaper() has been called improperly")

  pid_t pid = posix::error_response; // set value just in case
  int status = 0;
  while((pid = ::waitpid(pid_t(-1), &status, WNOHANG)) != posix::error_response) // get the next dead process (if there is one)... while the currently reaped process was valid
  {
    auto process_map_iter = process_map.find(pid); // find dead process
    if(process_map_iter != process_map.end()) // if the dead process exists...
    {
      Process* p = process_map_iter->second;
#ifndef ENABLE_PROCESS_EVENT_TRACKING
      Object::enqueue_copy(p->finished, posix::invalid_descriptor, EventData_t(EventFlags::ExitEvent, p->processId(), p->processId(), status, sig));
#endif
      EventBackend::remove(p->getStdOut());
      EventBackend::remove(p->getStdErr());
      posix::close(p->getStdOut());
      posix::close(p->getStdErr());
      posix::close(p->getStdIn());
      p->m_state = Process::State::Finished;
      process_map.erase(process_map_iter); // remove finished process from the process map
    }
  }
}

Process::Process(void) noexcept
  : m_state(State::Initializing)
{
  init_once();
  process_map.emplace(processId(), this); // add self to process map
#ifdef ENABLE_PROCESS_EVENT_TRACKING
  Object::connect(processId(), EventFlags::ExecEvent, started);
  Object::connect(processId(), EventFlags::ExitEvent, finished);
#endif
}

Process::~Process(void) noexcept
{
  if(m_state == State::Running)
#ifdef _XOPEN_SOURCE_EXTENDED
    sendSignal(posix::signal::Kill);
#else
    ::kill(processId(), 0);
#endif

  Object::disconnect(getStdOut(), EventFlags::Readable);
  Object::disconnect(getStdErr(), EventFlags::Readable);
  Object::disconnect(processId(), EventFlags::ExecEvent);
  Object::disconnect(processId(), EventFlags::ExitEvent);
  m_state = State::Invalid;
}

bool Process::write_then_read(void) noexcept
{
  if(m_iobuf.empty() || // ensure there is data to write
     !writeStdIn(m_iobuf) || // write data to pipe
     !waitReadStdOut(1000) || // wait to read pipe
     !readStdOut(m_iobuf) || // read data from pipe
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
  { return posix::signal::send(processId(), id, value); }

bool Process::invoke(void) noexcept
{
  flaw(m_state != State::Initializing, posix::severe, posix::error(std::errc::device_or_resource_busy), false,
       "Called Process::invoke() on an active process!")

  m_iobuf.reset();
  m_iobuf << command::invoke;

  if(!writeStdIn(m_iobuf))
    return false;

  m_state = State::Invalid;
  state();

#ifndef ENABLE_PROCESS_EVENT_TRACKING
  Object::enqueue_copy(started, posix::invalid_descriptor, EventData_t(EventFlags::ExecEvent, processId(), processId()));
#endif

  Object::connect(getStdOut(), EventFlags::Readable, stdoutMessage);
  Object::connect(getStdErr(), EventFlags::Readable, stderrMessage);

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
      flaw(::procstat(processId(), &data) == posix::error_response, posix::severe, m_state = State::Invalid, m_state,
           "Process %i does not exist.", processId()); // process _must_ exist
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
