#include "process.h"

// POSIX
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

// POSIX++
#include <cerrno>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <cassert>

// PDTK
#include <specialized/procstat.h>
#include <specialized/eventbackend.h>
#include <cxxutils/error_helpers.h>
#include <cxxutils/vterm.h>


static std::unordered_map<pid_t, Process*> process_map; // do not try to own Process memory

void Process::init_once(void) noexcept
{
  static bool first = true;
  if(first)
  {
    first = false;
    struct sigaction actions;
    actions.sa_handler = &handler;
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    flaw(::sigaction(SIGCHLD, &actions, nullptr) == posix::error_response,
         terminal::critical,
         std::exit(errno),,
         "Unable assign action to a signal: %s", std::strerror(errno))
  }
}

void Process::handler(int signum) noexcept
{
  flaw(signum != SIGCHLD,
       terminal::warning,
       posix::error(std::errc::invalid_argument),,
       "Process::reaper() has been called improperly")

  pid_t pid = posix::error_response; // set value just in case
  int status = 0;
  while((pid = ::waitpid(pid_t(-1), &status, WNOHANG)) > 0) // get the next dead process (if there is one)... while the currently reaped process was valid
  {
    auto process_map_iter = process_map.find(pid); // find dead process
    if(process_map_iter != process_map.end()) // if the dead process exists...
    {
      Process* p = process_map_iter->second;
      if(WIFEXITED(status))
      {
        EventBackend::remove(p->getStdOut(), EventBackend::SimplePollReadFlags);
        EventBackend::remove(p->getStdErr(), EventBackend::SimplePollReadFlags);
        posix::close(p->getStdOut());
        posix::close(p->getStdErr());
        posix::close(p->getStdIn());

        p->m_state = Process::State::Finished;
        if(WIFSIGNALED(status))
          Object::enqueue_copy(p->killed, p->processId(), posix::signal::EId(WTERMSIG(status)));
        else
          Object::enqueue_copy(p->finished, p->processId(), posix::error_t(WEXITSTATUS(status)));
        process_map.erase(process_map_iter); // remove finished process from the process map
      }
      else if(WIFSTOPPED(status))
      {
        p->m_state = Process::State::Stopped;
        Object::enqueue_copy(p->stopped, p->processId());
      }
      else if(WIFCONTINUED(status))
      {
        p->m_state = Process::State::Running;
        Object::enqueue_copy(p->started, p->processId());
      }
    }
  }
}

Process::Process(void) noexcept
  : m_state(State::Initializing)
{
  init_once();
  process_map.emplace(processId(), this); // add self to process map
}

Process::~Process(void) noexcept
{
  EventBackend::remove(getStdOut(), EventBackend::SimplePollReadFlags);
  EventBackend::remove(getStdErr(), EventBackend::SimplePollReadFlags);
}

bool Process::setOption(const std::string& name, const std::string& value) noexcept
{
  m_iobuf.reset();
  return !(m_iobuf << name << value).hadError() && writeStdIn(m_iobuf);
}

bool Process::sendSignal(posix::signal::EId id, int value) const noexcept
{
  return posix::signal::send(processId(), id, value);
}

bool Process::invoke(void) noexcept
{
  posix::success();

  flaw(m_state != State::Initializing,
       terminal::severe,
       posix::error(std::errc::device_or_resource_busy),
       false,
       "Called Process::invoke() on an active process!");

  EventBackend::add(getStdOut(), EventBackend::SimplePollReadFlags,
                    [this](posix::fd_t lambda_fd, native_flags_t) noexcept
                      { Object::enqueue(stdoutMessage, lambda_fd); });

  EventBackend::add(getStdErr(), EventBackend::SimplePollReadFlags,
                    [this](posix::fd_t lambda_fd, native_flags_t) noexcept
                      { Object::enqueue(stderrMessage, lambda_fd); });

  m_iobuf.reset();
  if((m_iobuf << "Launch").hadError() ||
     !writeStdIn(m_iobuf))
    return false;

  m_state = State::Invalid;
  if(state() == State::Running)
    Object::enqueue_copy(started, processId());

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
      flaw(::procstat(processId(), data) == posix::error_response && m_state != State::Finished,
           terminal::severe,
           m_state = State::Invalid,
           m_state,
           "Process %i does not exist.", processId()); // process must exist (rare case where process could exit durring this call is handled)
      switch (data.state)
      {
        case WaitingInterruptable:
        case WaitingUninterruptable:
          m_state = State::Waiting; break;
        case Zombie : m_state = State::Zombie ; break;
        case Stopped: m_state = State::Stopped; break;
        case Running: m_state = State::Running; break;
        default: break; // invalid state (process exited before it could be stat'd
      }
  }
  return m_state;
}
