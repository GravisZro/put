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
#include <object.h>
#include <cxxutils/posix_helpers.h>


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

  // for string literals
  void  setWorkingDirectory(const char* dir) { m_workingdir = dir; }
  void  setExecutable(const char* executable) { m_executable = executable; }
  void  setArguments(std::vector<const char*>& arguments);
  void  setEnvironment(std::vector<std::pair<const char*, const char*>>& environment);
  void  setEnvironmentVariable(const char* name, const char* value) { m_environment.insert_or_assign(name, value); }

  // for strings
  void  setWorkingDirectory(const std::string& dir) { m_workingdir = dir; }
  void  setExecutable(const std::string& executable) { m_executable = executable; }
  void  setArguments(const std::vector<std::string>& arguments) { m_arguments = arguments; }
  void  setEnvironment(const std::unordered_map<std::string, std::string>& environment)  { m_environment = environment; }
  void  setEnvironmentVariable(const std::string& name, const std::string& value) { m_environment.insert_or_assign(name, value); }

  pid_t id   (void) const { return m_pid; }
  State state(void);

  void  start     (void);
  bool  sendSignal(posix::signal::EId id) const;

  void  stop      (void) const { sendSignal(posix::signal::Stop      ); }
  void  resume    (void) const { sendSignal(posix::signal::Resume    ); }

  void  quit      (void) const { sendSignal(posix::signal::Quit      ); }
  void  terminate (void) const { sendSignal(posix::signal::Terminate ); }
  void  kill      (void) const { sendSignal(posix::signal::Kill      ); }


  signal<> started;
  signal<Error, std::errc> error;
  signal<int> finished;

private:
  std::string m_executable;
  std::vector<std::string> m_arguments;
  std::string m_workingdir;
  std::unordered_map<std::string, std::string> m_environment;
  pid_t m_pid;
  State m_state;
  posix::fd_t m_stdin;
  posix::fd_t m_stdout;
  posix::fd_t m_stderr;
};

#endif // PROCESS_H
