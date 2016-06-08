#include "process.h"

#include "cxxutils/cstringarray.h"

Process::Process(void)
  : m_pid(0), m_state(NotStarted)
{ }

Process::~Process(void)
{
  m_pid = 0;
  m_state = NotStarted;
}

bool Process::start(void)
{
  m_arguments.insert(m_arguments.begin(), m_executable);
  CStringArray argv(m_arguments);
  CStringArray envv(m_environment, [](const std::pair<std::string, std::string>& p) { return p.first + '=' + p.second; });

  int err = ::posix_spawn(&m_pid, argv[0], actions, attr, argv, envv);

  if(err == posix::success_response)
    m_state = Running;
  else
    Object::enqueue_copy(error, FailedToStart, static_cast<std::errc>(err));
  return err == posix::success_response;
}

bool Process::sendSignal(posix::signal::EId id) const
{
  if(m_pid)
    return posix::signal::send(m_pid, id);
  return false;
}
