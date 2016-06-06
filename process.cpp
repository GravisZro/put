#include "process.h"


Process::Process(void)
  : m_pid(0), m_state(NotStarted)
{ }

Process::~Process(void)
{
  m_pid = 0;
  m_state = NotStarted;
}

void Process::setArguments(std::vector<const char*> arguments)
{
  m_arguments.clear();
  for(auto arg : arguments)
    m_arguments.emplace_back(arg);
}

void Process::setEnvironment(std::vector<std::pair<const char*, const char*>> environment)
{
  environment.clear();
  for(auto& pos : environment)
    setEnvironmentVariable(pos.first, pos.second);
}


Process::State Process::state(void)
{

}

void Process::start(void)
{
  /*
  posix_spawn_file_actions_t
  int posix_spawn(&m_pid, m_executable.c_str(),
        pid_t *restrict pid, const char *restrict path,
                          const posix_spawn_file_actions_t *file_actions,
                          const posix_spawnattr_t *restrict attrp,
                          char *const argv[restrict], char *const envp[restrict]);
                          */
}

bool Process::sendSignal(posix::ESignal signum) const
{
  if(m_pid)
    return posix::signal::send(m_pid, signum, 0);
  return false;
}
