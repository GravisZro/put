#ifndef PROCSTAT_H
#define PROCSTAT_H

// STL
#include <string>
#include <vector>

// POSIX++
#include <cstdint>

// PDTK
#include <cxxutils/posix_helpers.h>


enum ExecutionState : char
{
  Invalid = 0,                  // invalid state (only used internally)
  Running = 'R',                // actively executing
  WaitingInterruptable = 'S',   // sleeping in an interruptible wait
  WaitingUninterruptable = 'D', // sleeping in an uninterruptible wait
  Zombie = 'Z',                 // zombie process
  Stopped = 'T',                // stopped/tracing execution
};
typedef uint32_t signals_t;

struct process_state_t
{
  pid_t process_id;         // process id
  std::string name;         // process name
  std::string executable;   // full path to executable
  std::vector<std::string> arguments; // arguments of launched executable
  ExecutionState state;     // process execution state
  pid_t parent_process_id;  // process id of the parent process
  pid_t process_group_id;   // process group id
  pid_t session_id;         // session id

  int tty; // number of the tty the process running on

  signals_t signals_pending;
  signals_t signals_blocked;
  signals_t signals_ignored;
  signals_t signals_caught;

  int priority_value; // nice value
};

posix::error_t procstat(pid_t pid, process_state_t& data) noexcept;

#endif // PROCSTAT_H
