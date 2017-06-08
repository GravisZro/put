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
#include <cxxutils/cstringarray.h>

// for debug
#include <iostream>

namespace static_secrets
{
  static std::unordered_map<pid_t, Process*> process_map; // do not try to own Process memory

  void reaper(int sig)
  {
    assert(sig == SIGCHLD); // there should only be one way to get here, so be sure there is no funny business
    pid_t pid = posix::error_response; // set value just in case
    int status = 0;
    while((pid = ::waitpid(pid_t(-1), &status, WNOHANG)) != posix::error_response) // get the next dead process (if there is one)... while the currently reaped process was valid
    {
      auto process_map_iter = process_map.find(pid); // find dead process
      if(process_map_iter != process_map.end()) // if the dead process exists...
      {
        Object::enqueue(process_map_iter->second->finished, status); // emit finished signal with errno code
        process_map_iter->second->m_state = Process::State::Finished;
        process_map.erase(process_map_iter); // remove finished process from the process map
      }
    }
  }

  static void init_once(void) noexcept
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
        ::perror("An \"impossible\" situation has occurred.");
        ::exit(1);
      }
    }
  }
}

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

static inline int get_errno(int value)
{ return value == posix::success_response ? posix::success_response : errno; }

Process::Process(void) noexcept
  : m_state(State::Invalid),
    m_error(Error::None)
{
  if(PipedFork::isChildProcess()) // if this is the forked off process loader stub (loads executable over this process)
  {
    // asserts will make it known where it failed via stderr

    EventBackend::watch(m_read, EventFlags::Readable);
    //EventBackend::watch(m_write, EventFlags::Readable);

    std::string exefile;
    std::string workingdir;
    std::vector<std::string> arguments;

    arguments.reserve(256);

    uint8_t cmd = 0;

    for(;;) // loop forever
    {
      if(EventBackend::getevents()) // wait for new data indefinitely
      {
        for(const auto& pos : EventBackend::results)
        {
          assert(pos.first == m_read && (pos.second & EventFlags::Readable));
          assert(!m_iobuf.hadError());
          if(!PipedFork::read(m_iobuf)) // unable to read
            ::exit(errno); // why read() failed

          if(m_iobuf.empty() || (m_iobuf >> cmd).hadError()) // extraction problem (unrecoverable)
          {
            m_iobuf.reset();
            m_iobuf << int(EINVAL);
            if(PipedFork::write(m_iobuf))
              continue;
            ::exit(ENOTRECOVERABLE);
          }

          switch(command(cmd))
          {
            case command::invoke: // invoke the executable with the previous specified commands
            {
              m_iobuf.reset();
              if(!exefile.empty()) // if executable name was specified explicitly
                arguments.insert(arguments.begin(), exefile); // insert executable name to the beginning of the argument list

              CStringArray argv(arguments); // buffer for execv
              ::execv(argv[0], argv);
              ::exit(errno);
            }

            case command::executable: // specify executable name explicitly
            {
              struct stat statbuf;
              m_iobuf >> exefile;
              if(::stat(exefile.c_str(), &statbuf) == posix::error_response)
                m_iobuf << errno;
              else if(statbuf.st_mode ^ S_IFREG)
                m_iobuf << static_cast<int>(std::errc::permission_denied);
              else if(statbuf.st_mode ^ S_IEXEC)
                m_iobuf << static_cast<int>(std::errc::permission_denied);
              else
                m_iobuf << posix::success_response;
              break;
            }

            case command::arguments: // specify arguments
            {
              arguments.clear();
              std::string arg;
              while(!(m_iobuf >> arg).hadError())
                arguments.push_back(arg);
              m_iobuf.reset();
              m_iobuf << (arguments.empty() ? posix::error_response : posix::success_response);
              break;
            }

            case command::environment: // specify one or more environment variables
            case command::environmentvar: // specify one environment variable
            {
              int rval = posix::success_response;
              std::string key, value;
              while(!(m_iobuf >> key  ).hadError() &&
                    !(m_iobuf >> value).hadError() &&
                    rval == posix::success_response)
                rval = ::setenv(key.data(), value.data(), 1);
              m_iobuf.reset();
              m_iobuf << get_errno(rval);
              break;
            }

            case command::workingdir: // specify the working directory
            {
              struct stat statbuf;
              m_iobuf >> workingdir;

              if(::stat(workingdir.c_str(), &statbuf) == posix::error_response)
                m_iobuf << errno;
              else if(statbuf.st_mode ^ S_IFDIR)
                m_iobuf << int(std::errc::permission_denied);
              else if(statbuf.st_mode ^ S_IEXEC)
                m_iobuf << int(std::errc::permission_denied);
              else
                m_iobuf << posix::success_response;
              break;
            }

            case command::priority: // specify process priority
            {
              int priority;
              m_iobuf >> priority;
              m_iobuf << get_errno(::setpriority(PRIO_PROCESS, getpid(), priority)); // set priority
              break;
            }

            case command::uid: // specify process user id number
            {
              uid_t uid;
              m_iobuf >> uid;
              m_iobuf << get_errno(::setuid(uid));
              break;
            }

            case command::gid: // specify process group id number
            {
              gid_t gid;
              m_iobuf >> gid;
              m_iobuf << get_errno(::setgid(gid));
              break;
            }

            case command::euid: // specify process effective user id number
            {
              uid_t euid;
              m_iobuf >> euid;
              m_iobuf << get_errno(::seteuid(euid));
              break;
            }

            case command::egid: // specify process effective group id number
            {
              gid_t egid;
              m_iobuf >> egid;
              m_iobuf << get_errno(::setegid(egid));
              break;
            }

            case command::resource: // specify a limit
            {
              int limit_id;
              rlimit val;
              m_iobuf >> limit_id;
              m_iobuf >> val.rlim_cur;
              m_iobuf >> val.rlim_max;
              m_iobuf << get_errno(::setrlimit(limit_id, &val));
              break;
            }

            default: // bad request code
              m_iobuf << EBADRQC;
              break;
          }

          if(PipedFork::write(m_iobuf)) // write the result
            m_iobuf.reset();
        }
      }
    }
  }
  else // if this is initilizing the parent process
  {
    static_secrets::init_once(); // only executes for the first instance of a Process type
    static_secrets::process_map.emplace(PipedFork::id(), this); // add self to process map
    m_state = State::Initializing;
  }
}

Process::~Process(void) noexcept
{
  EventBackend::remove(m_read);
  EventBackend::remove(m_write);

  if(m_state == State::Running)
    sendSignal(posix::signal::Kill);

  m_state = State::Invalid;
}

bool Process::write_then_read(void) noexcept
{
  if(m_iobuf.empty() || // ensure there is data to write
     !PipedFork::write(m_iobuf) || // write data to pipe
     !PipedFork::waitWrite(100) || // wait for a response to be written
     !PipedFork::read(m_iobuf) || // read data from pipe
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
  return posix::signal::send(PipedFork::id(), id, value);
}

bool Process::start(void) noexcept
{
  assert(m_state == State::Initializing);

  m_iobuf.reset();
  m_iobuf << command::invoke;

  if(!PipedFork::write(m_iobuf))
    return false;

  process_state_t data;
  if(!::procstat(PipedFork::id(), data))
  {
    m_state = State::Invalid;
    return false;
  }

  m_state = State::Running;

  Object::enqueue(Process::started);
  return errno == posix::success_response;
}

Process::State Process::state(void) noexcept
{
  if(m_state != State::Finished) // if the process hasn't been reaped already
  {
    process_state_t data;
    if(!::procstat(PipedFork::id(), data)) // check if running
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
