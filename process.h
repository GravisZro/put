#ifndef PROCESS_H
#define PROCESS_H

// POSIX
#include <sys/types.h>
#include <signal.h>

// STL
#include <string>
#include <vector>
#include <system_error>
#include <utility>
#include <initializer_list>
#include <vector>
#include <unordered_map>

// PDTK
#include "object.h"
#include "cxxutils/posix_helpers.h"
#include "cxxutils/ranged.h"


class Process : public Object
{
public:
  enum State
  {
    NotStarted = 0, // has not been started
    Starting,       // starting, but not yet been invoked
    Running,        // running and is ready
    Finished,       // finshed running and exited
  };

  enum Error
  {
    FailedToStart = 0,  // failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.
    Crashed,            // crashed some time after starting successfully.
    Timedout,           // The last waitFor...() function timed out.
    WriteError,         // An error occurred when attempting to write to the process.
    ReadError,          // An error occurred when attempting to read from the process.
    UnknownError,       // An unknown error occurred. This is the default return value of error().
  };

  Process(void);
 ~Process(void);

  void  setArguments(const std::vector<std::string>& arguments) { m_arguments = arguments; }
  void  setEnvironment(const std::unordered_map<std::string, std::string>& environment)  { m_environment = environment; }
  void  setEnvironmentVariable(const std::string& name, const std::string& value) { m_environment.emplace(name, value); }

  bool  setWorkingDirectory(const std::string& dir);
  bool  setExecutable(const std::string& executable);
  void  setUID(uid_t id) { m_uid = id; }
  void  setGID(gid_t id) { m_gid = id; }
  bool  setPriority(int nval) { m_priority = nval; return m_priority.isValid(); }

  pid_t id   (void) const { return m_pid; }
  State state(void) const { return m_state; }

  bool  start     (void);
  bool  sendSignal(posix::signal::EId id) const;

  void  stop      (void) const { sendSignal(posix::signal::Stop     ); }
  void  resume    (void) const { sendSignal(posix::signal::Resume   ); }

  void  quit      (void) const { sendSignal(posix::signal::Quit     ); }
  void  terminate (void) const { sendSignal(posix::signal::Terminate); }
  void  kill      (void) const { sendSignal(posix::signal::Kill     ); }

  signal<> started;
  signal<Error, std::errc> error;
  signal<int> finished;

private:
  std::string m_executable;
  std::vector<std::string> m_arguments;
  std::string m_workingdir;
  std::unordered_map<std::string, std::string> m_environment;
  State m_state;
  pid_t m_pid;
  uid_t m_uid;
  gid_t m_gid;
  ranged<int, -20, 20> m_priority;
  posix::fd_t m_stdout;
  posix::fd_t m_stderr;
};

#endif // PROCESS_H
