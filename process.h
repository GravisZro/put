#ifndef PROCESS_H
#define PROCESS_H

// POSIX
#include <sys/types.h>
#include <sys/resource.h>
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
    NoError = 0,
    FailedToStart,  // failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.
    Crashed,        // crashed some time after starting successfully.
    Timedout,       // The last waitFor...() function timed out.
    WriteError,     // An error occurred when attempting to write to the process.
    ReadError,      // An error occurred when attempting to read from the process.
    UnknownError,   // An unknown error occurred. This is the default return value of error().
  };

  enum Resource : int
  {
    CoreDumpSize    = RLIMIT_CORE,    // Limit on size of core file.
    CPUTime         = RLIMIT_CPU,     // Limit on CPU time per process.
    DataSegmentSize = RLIMIT_DATA,    // Limit on data segment size.
    FileSize        = RLIMIT_FSIZE,   // Limit on file size.
    FilesOpen       = RLIMIT_NOFILE,  // Limit on number of open files.
    StackSize       = RLIMIT_STACK,   // Limit on stack size.
    VirtualMemory   = RLIMIT_AS,      // Limit on address space size.
  };

  Process(void) noexcept;
 ~Process(void) noexcept;

  void setArguments(const std::vector<std::string>& arguments) noexcept;
  void setEnvironment(const std::unordered_map<std::string, std::string>& environment) noexcept { m_environment = environment; }
  void setEnvironmentVariable(const std::string& name, const std::string& value) noexcept { m_environment[name] = value; }
  void setResourceLimit(Resource which, rlim_t limit) noexcept { m_limits[which] = rlimit{RLIM_SAVED_CUR, limit}; }

  bool setWorkingDirectory(const std::string& dir) noexcept;
  bool setExecutable(const std::string& executable) noexcept;
  bool setUserID(uid_t id) noexcept;
  bool setGroupID(gid_t id) noexcept;
  bool setEffectiveUserID(uid_t id) noexcept;
  bool setEffectiveGroupID(gid_t id) noexcept;
  bool setPriority(int nval) noexcept;


  posix::fd_t getStdOut(void) const noexcept { return m_stdout; }
  posix::fd_t getStdErr(void) const noexcept { return m_stderr; }

  pid_t id   (void) const noexcept { return m_pid; }
  State state(void) const noexcept { return m_state; }

  bool start     (void) noexcept;
  bool sendSignal(posix::signal::EId id, int value = 0) const noexcept;

  void stop      (void) const noexcept { sendSignal(posix::signal::Stop     ); }
  void resume    (void) const noexcept { sendSignal(posix::signal::Resume   ); }

  void quit      (void) const noexcept { sendSignal(posix::signal::Quit     ); }
  void terminate (void) const noexcept { sendSignal(posix::signal::Terminate); }
  void kill      (void) const noexcept { sendSignal(posix::signal::Kill     ); }


  signal<> started;
  signal<Error, std::errc> error;
  signal<int> finished;

private:
  std::string m_executable;
  std::vector<std::string> m_arguments;
  std::string m_workingdir;
  std::unordered_map<std::string, std::string> m_environment;
  State m_state;
  Error m_error;
  pid_t m_pid;
  uid_t m_uid;
  gid_t m_gid;
  uid_t m_euid;
  gid_t m_egid;
  int m_priority;
  std::unordered_map<Resource, rlimit> m_limits;
  posix::fd_t m_stdout;
  posix::fd_t m_stderr;
};

#endif // PROCESS_H
