#ifndef PROCESS_H
#define PROCESS_H

// POSIX
#include <sys/types.h>
#include <sys/resource.h>
#include <signal.h>

// POSIX++
#include <csignal>

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

  Process(void) noexcept;
 ~Process(void) noexcept;

  bool setOption(const std::string& name, const std::string& value) noexcept;

  State state    (void) noexcept;

  bool invoke    (void) noexcept;

  bool sendSignal(posix::signal::EId id, int value = 0) const noexcept;

  void stop      (void) const noexcept { sendSignal(posix::signal::Stop     ); }
  void resume    (void) const noexcept { sendSignal(posix::signal::Resume   ); }

  void quit      (void) const noexcept { sendSignal(posix::signal::Quit     ); }
  void terminate (void) const noexcept { sendSignal(posix::signal::Terminate); }
  void kill      (void) const noexcept { sendSignal(posix::signal::Kill     ); }

  signal<posix::fd_t> stdoutMessage;
  signal<posix::fd_t> stderrMessage;
  signal<pid_t> started;
  signal<pid_t> stopped;
  signal<pid_t, posix::error_t> finished;
  signal<pid_t, posix::signal::EId> killed;
private:
  vfifo m_iobuf;
  State m_state;

  static void handler(int signum) noexcept;
  static void init_once(void) noexcept;
};

#endif // PROCESS_H
