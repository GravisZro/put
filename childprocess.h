#ifndef PROCESS_H
#define PROCESS_H

// POSIX
#include <sys/resource.h>

// STL
#include <string>
#include <vector>
#include <utility>
#include <initializer_list>
#include <vector>
#include <unordered_map>

// PUT
#include <object.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/posix_helpers.h>
#include <cxxutils/pipedspawn.h>

class ChildProcess : public Object,
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

  ChildProcess(void) noexcept;
 ~ChildProcess(void) noexcept;

  bool setOption(const std::string& name, const std::string& value) noexcept;

  State state    (void) noexcept;

  bool invoke    (void) noexcept;

  bool sendSignal(posix::Signal::EId id, int value = 0) const noexcept;

  void stop      (void) const noexcept { sendSignal(posix::Signal::Stop     ); }
  void resume    (void) const noexcept { sendSignal(posix::Signal::Resume   ); }

  void quit      (void) const noexcept { sendSignal(posix::Signal::Quit     ); }
  void terminate (void) const noexcept { sendSignal(posix::Signal::Terminate); }
  void kill      (void) const noexcept { sendSignal(posix::Signal::Kill     ); }

  signal<posix::fd_t> stdoutMessage;
  signal<posix::fd_t> stderrMessage;
  signal<pid_t> started;
  signal<pid_t> stopped;
  signal<pid_t, posix::error_t> finished;
  signal<pid_t, posix::Signal::EId> killed;
private:
  vfifo m_iobuf;
  State m_state;

  static void handler(int signum) noexcept;
  static void init_once(void) noexcept;
};

#endif // PROCESS_H
