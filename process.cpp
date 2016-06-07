#include "process.h"


Process::Process(void)
  : m_pid(0), m_state(NotStarted)
{ }

Process::~Process(void)
{
  m_pid = 0;
  m_state = NotStarted;
}
/*
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
*/


bool Process::start(void)
{
  std::vector<std::string> environment;
  std::vector<const char*> argv;
  std::vector<const char*> envp;
  bool result;

  posix::spawn::Attributes attr;
  posix::spawn::FileActions actions;

  for(auto& env : m_environment) { environment.push_back(env.first + "=" + env.second); } // create environment string
  for(auto& env : environment  ) { envp.push_back(env.c_str()); } // copy pointers to environment string data
  for(auto& arg : m_arguments  ) { argv.push_back(arg.c_str()); } // copy pointers to argument string data

  result = ::posix_spawn(&m_pid, m_executable.c_str(),
                         actions, attr,
                         (char* const*)argv.data(),
                         (char* const*)envp.data()) == posix::success_response;
  if(result)
    m_state = Running;
  else
    Object::enqueue_copy(error, FailedToStart, static_cast<std::errc>(errno));
  return result;
}

bool Process::sendSignal(posix::signal::EId id) const
{
  if(m_pid)
    return posix::signal::send(m_pid, id, 0);
  return false;
}
