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

namespace static_secrets
{
  static std::unordered_map<pid_t, std::unique_ptr<Process>> process_map;

  static void reaper(int sig)
  {
    (void)sig;
    int saved_errno = errno;
    pid_t pid = posix::error_response;
    while((pid = ::waitpid((pid_t)(-1), 0, WNOHANG)) != posix::error_response)
    {
      auto process_map_iter = process_map.find(pid);
      if(process_map_iter != process_map.end())
      {
        Object::enqueue(process_map_iter->second->finished, errno);
        //process_map.erase(process_map_iter);
      }
    }
    errno = saved_errno;
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

      if(::sigaction(SIGCHLD, &actions, 0) == posix::error_response)
      {
        ::perror(0);
        ::exit(1);
      }
    }
  }
}

enum command : uint8_t
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

static inline vqueue& operator << (vqueue& vq, command cmd) noexcept
  { return vq << static_cast<uint8_t>(cmd); }

static inline int get_errno(int value)
  { return value == posix::success_response ? posix::success_response : errno; }

Process::Process(void) noexcept
  : m_state(State::Invalid)
{
  if(PipedFork::isChildProcess())
  {
    // asserts will make it known where it failed via stderr

    EventBackend::watch(m_read, EventFlags_e::Read);
    EventBackend::watch(m_write, EventFlags_e::Read);

    std::string exefile;
    std::string workingdir;
    std::vector<std::string> arguments;
//    std::unordered_map<std::string, std::string> environment;
    std::unordered_map<Resource, rlimit> limits;

    arguments.reserve(256);
//    environment.reserve(256);
    limits.reserve(8);

    uint8_t cmd;

    for(;;)
    {
      if(EventBackend::invoke())
      {
        for(const auto& pos : EventBackend::results())
        {
          if(pos.first == m_read && (pos.second & EventFlags_e::Read))
          {
            if(!PipedFork::read(m_iobuf))
              ::exit(ENOMEM);
            if(m_iobuf.empty() || (m_iobuf >> cmd).hadError())
            {
              m_iobuf.reset();
              m_iobuf << EINVAL;
              if(PipedFork::write(m_iobuf))
                continue;
              ::exit(ENOTRECOVERABLE);
            }

            switch(cmd)
            {
              case command::invoke:
              {
//                environment["PATH"] = "/bin:/usr/bin:/usr/local/bin";

                if(!exefile.empty())
                  arguments.insert(arguments.begin(), exefile);

                CStringArray argv(arguments);
//                CStringArray envv(environment, [](const std::pair<std::string, std::string>& p) { return p.first + '=' + p.second; });

                ::execv(argv[0], argv);
//                ::execve(argv[0], argv, envv);

                ::exit(errno);
                //m_iobuf << get_errno(::execve(argv[0], argv, envv));
              }

              case command::executable:
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
              case command::arguments:
              {
                arguments.clear();
                std::string arg;
                while(!(m_iobuf >> arg).hadError())
                  arguments.push_back(arg);
                m_iobuf << (arguments.empty() ? posix::error_response : posix::success_response);
                break;
              }
              case command::environment:
//                environment.clear();
              case command::environmentvar:
              {
                int rval = posix::success_response;
                std::string key, value;
                while(!(m_iobuf >> key  ).hadError() &&
                      !(m_iobuf >> value).hadError() &&
                      rval == posix::success_response)
                  rval = ::setenv(key.data(), value.data(), 1);
                m_iobuf << get_errno(rval);
                break;
              }
              case command::workingdir:
              {
                struct stat statbuf;
                m_iobuf >> workingdir;

                if(::stat(workingdir.c_str(), &statbuf) == posix::error_response)
                  m_iobuf << errno;
                else if(statbuf.st_mode ^ S_IFDIR)
                  m_iobuf << static_cast<int>(std::errc::permission_denied);
                else if(statbuf.st_mode ^ S_IEXEC)
                  m_iobuf << static_cast<int>(std::errc::permission_denied);
                else
                  m_iobuf << posix::success_response;
                break;
              }
              case command::priority:
              {
                int priority;
                m_iobuf >> priority;
                m_iobuf << get_errno(::setpriority(PRIO_PROCESS, getpid(), priority)); // set priority
                break;
              }
              case command::uid:
              {
                uid_t uid;
                m_iobuf >> uid;
                m_iobuf << get_errno(::setuid(uid));
                break;
              }
              case command::gid:
              {
                gid_t gid;
                m_iobuf >> gid;
                m_iobuf << get_errno(::setgid(gid));
                break;
              }
              case command::euid:
              {
                uid_t euid;
                m_iobuf >> euid;
                m_iobuf << get_errno(::seteuid(euid));
                break;
              }
              case command::egid:
              {
                gid_t egid;
                m_iobuf >> egid;
                m_iobuf << get_errno(::setegid(egid));
                break;
              }
              case command::resource:
              {
                static_assert(sizeof(Resource) == sizeof(int), "size error");
                Resource id;
                rlimit val;
                m_iobuf >> *reinterpret_cast<int*>(&id);
                m_iobuf >> val.rlim_cur;
                m_iobuf >> val.rlim_max;
                m_iobuf << get_errno(::setrlimit(id, &val));
                break;
              }
              default:
                break;
            }
            if(PipedFork::write(m_iobuf))
              m_iobuf.reset();
          }
          else
          {
            // ?!
          }
        }
      }
    }
  }
  else
  {
    static_secrets::init_once();
    static_secrets::process_map.emplace(PipedFork::id(), this);
    m_state = State::Initializing;
  }
}

Process::~Process(void) noexcept
{
  if(m_state == State::Running)
    sendSignal(posix::signal::Kill);

  m_state = State::Invalid;
}

bool Process::write_then_read(void) noexcept
{
  if(!m_iobuf.empty() ||
     !PipedFork::write(m_iobuf) ||
     !PipedFork::read(m_iobuf) ||
     (m_iobuf >> errno).hadError())
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
  m_iobuf << command::resource << static_cast<int>(which) << static_cast<rlim_t>(RLIM_SAVED_CUR) << limit;
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
#include <iostream>
bool Process::start(void) noexcept
{
  if(m_state != State::Initializing)
    return false;

  m_iobuf.reset();
  m_iobuf << command::invoke;

  std::cout << "pid  : " << PipedFork::id() << std::endl;
  std::cout << "read : " << m_read << std::endl;
  std::cout << "write: " << m_write << std::endl;
  std::cout << "size : " << m_iobuf.size() << std::endl;

  if(!PipedFork::write(m_iobuf))
  {
    assert(false);
    return false;
  }

  if(::usleep(100) == posix::error_response)
  {
    assert(false);
    return false;
  }

  process_state_t data;
  if(!::procstat(PipedFork::id(), data))
  {
    m_state = State::Invalid;
    assert(false);
    return false;
  }

  m_state = State::Running;

  Object::enqueue(Process::started);
  return errno == posix::success_response;
}

Process::State Process::state(void) noexcept
{
  process_state_t data;
  if(!::procstat(PipedFork::id(), data)) // check if running
  {
    m_state = State::Invalid;
    return m_state;
  }
  else if(m_state != State::Initializing)
  {
    switch (data.state)
    {
      case WaitingInterruptable:
      case WaitingUninterruptable:
                    m_state = State::Waiting; break;
      case Zombie : m_state = State::Zombie ; break;
      case Stopped: m_state = State::Stopped; break;
      default:break;
    }
  }

  return m_state;
}
