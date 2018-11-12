#ifndef PROCSTAT_H
#define PROCSTAT_H

// STL
#include <string>
#include <vector>

// POSIX++
#include <cstdint>

// POSIX
#include <time.h>

// PUT
#include <cxxutils/posix_helpers.h>

enum ExecutionState : char
{
  Invalid = '\0',               // invalid state (if procstat fails)
  Running = 'R',                // actively executing
  WaitingInterruptable = 'S',   // sleeping in an interruptible wait
  WaitingUninterruptable = 'D', // sleeping in an uninterruptible wait
  Zombie = 'Z',                 // zombie process
  Stopped = 'T',                // stopped/tracing execution
};

typedef int32_t segsz_t; // memory segment size

struct process_state_t
{
  std::string name;         // process name
  std::string executable;   // full path to executable
  std::vector<std::string> arguments; // arguments of launched executable
  ExecutionState state;     // process execution state
  uid_t user_id;            // process associated user id
  gid_t group_id;           // process associated group id
  pid_t process_id;         // process id
  pid_t parent_process_id;  // process id of the parent process
  pid_t process_group_id;   // process group id
  pid_t session_id;         // session id

  dev_t tty_device;         // device id of the tty device the process is running on

  sigset_t signals_pending; // signals pending
  sigset_t signals_blocked; // signals blocked
  sigset_t signals_ignored; // signals ignored
  sigset_t signals_caught;  // signals caught

  int priority_value;
  int8_t nice_value;        // nice value

  timeval start_time;       // process start time
  timeval user_time;        // process user time spent
  timeval system_time;      // process system time spent

  struct memory_sizes_t
  {
    segsz_t rss;    // resident set size (pages)
    segsz_t image;  // process image size (pages)
    segsz_t text;   // text size (pages)
    segsz_t data;   // data size (pages)
    segsz_t stack;  // stack size (pages)
    ssize_t page;   // page size (bytes)
  } memory_size;
};

bool procstat(pid_t pid, process_state_t& data) noexcept;

#endif // PROCSTAT_H
