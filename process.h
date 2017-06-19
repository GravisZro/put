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
#include <iostream>

// PDTK
#include <object.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/posix_helpers.h>
#include <cxxutils/pipedspawn.h>

class Process : public Object,
                public PipedSpawn
{
public:
  enum class State
  {
    Invalid = 0,  // process doesn't exist
    Initializing, // process exists but executable has not yet been loaded
    Running,      // process is running and active
    Waiting,      // process is running but asleep
    Stopped,      // process was running but execution has been stopped before completion
    Zombie,       // process has become a zombie process
    Finished,     // process executed and exited
  };

  enum class Error
  {
    None = 0,       // No error has occurred. This is the default return value of error().
    Unknown,        // An unknown error occurred.
    FailedToStart,  // process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.
    Crashed,        // process crashed some time after starting successfully.
    Killed,         // process was killed some time after starting successfully.
    Timedout,       // The last waitFor...() function timed out.
    Write,          // An error occurred when attempting to write to the process.
    Read,           // An error occurred when attempting to read from the process.
  };

  enum class Resource : int
  {
    CoreDumpSize    = RLIMIT_CORE,    // Limit on size of core file.
    CPUTime         = RLIMIT_CPU,     // Limit on CPU time per process.
    DataSegmentSize = RLIMIT_DATA,    // Limit on data segment size.
    FileSize        = RLIMIT_FSIZE,   // Limit on file size.
    FilesOpen       = RLIMIT_NOFILE,  // Limit on number of open files.
    StackSize       = RLIMIT_STACK,   // Limit on stack size.
    VirtualMemory   = RLIMIT_AS,      // Limit on address space size.
  };
  static_assert(sizeof(Resource) == sizeof(int), "size error");

  Process(void) noexcept;
 ~Process(void) noexcept;

  bool setArguments(const std::vector<std::string>& arguments) noexcept;
  bool setEnvironment(const std::unordered_map<std::string, std::string>& environment) noexcept;
  bool setEnvironmentVariable(const std::string& name, const std::string& value) noexcept;
  bool setResourceLimit(Resource which, rlim_t limit) noexcept;

  bool setWorkingDirectory(const std::string& dir) noexcept;
  bool setExecutable(const std::string& executable) noexcept;
  bool setUserID(uid_t id) noexcept;
  bool setGroupID(gid_t id) noexcept;
  bool setEffectiveUserID(uid_t id) noexcept;
  bool setEffectiveGroupID(gid_t id) noexcept;
  bool setPriority(int nval) noexcept;

  State state(void) noexcept;
  Error error(void) const noexcept { return m_error; }

  bool invoke    (void) noexcept;
  bool sendSignal(posix::signal::EId id, int value = 0) const noexcept;

  void stop      (void) const noexcept { sendSignal(posix::signal::Stop     ); }
  void resume    (void) const noexcept { sendSignal(posix::signal::Resume   ); }

  void quit      (void) const noexcept { sendSignal(posix::signal::Quit     ); }
  void terminate (void) const noexcept { sendSignal(posix::signal::Terminate); }
  void kill      (void) const noexcept { sendSignal(posix::signal::Kill     ); }

  signal<posix::fd_t, EventData_t> stdoutMessage;
  signal<posix::fd_t, EventData_t> stderrMessage;
  signal<posix::fd_t, EventData_t> started;
  signal<posix::fd_t, EventData_t> finished;
private:
  bool write_then_read(void) noexcept;
  vfifo m_iobuf;

  State m_state;
  Error m_error;
};

#endif // PROCESS_H
