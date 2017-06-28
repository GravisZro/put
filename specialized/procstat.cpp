#include "procstat.h"

// PDTK
#include <cxxutils/posix_helpers.h>

#if defined(__linux__)
// C++
#include <cstdio>

// Linux
#include <linux/limits.h>

bool procstat(pid_t pid, process_state_t& data) noexcept
{
  static_assert(sizeof(gid_t) == sizeof(int), "size error");
  static_assert(sizeof(uid_t) == sizeof(int), "size error");
  static_assert(sizeof(pid_t) == sizeof(int), "size error");

  data.filename.reserve(PATH_MAX);

  struct procinfo_t
  {
//    pid_t         pid;                      // The process id.
//    char          name[PATH_MAX];           // The filename of the executable
//    char          state;                    // R is running, S is sleeping, D is sleeping in an uninterruptible wait, Z is zombie, T is traced or stopped
    uid_t         euid;                     // effective user id
    gid_t         egid;                     // effective group id
//    pid_t         ppid;                     // The pid of the parent.
//    pid_t         pgrp;                     // The pgrp of the process.
//    pid_t         session;                  // The session id of the process.
//    int           tty;                      // The tty the process uses
    int           tpgid;                    // (too long)
    unsigned int	flags;                    // The flags of the process.
    unsigned int	minflt;                   // The number of minor faults
    unsigned int	cminflt;                  // The number of minor faults with childs
    unsigned int	majflt;                   // The number of major faults
    unsigned int  cmajflt;                  // The number of major faults with childs
    int           utime;                    // user mode jiffies
    int           stime;                    // kernel mode jiffies
    int           cutime;                   // user mode jiffies with childs
    int           cstime;                   // kernel mode jiffies with childs
    int           counter;                  // process's next timeslice
//    int           priority;                 // the standard nice value, plus fifteen
    unsigned int  timeout;                  // The time in jiffies of the next timeout
    unsigned int  itrealvalue;              // The time before the next SIGALRM is sent to the process
    int           starttime;                // Time the process started after system boot

    unsigned int  vsize;                    // Virtual memory size
    unsigned int  rss;                      // Resident Set Size
    unsigned int  rlim;                     // Current limit in bytes on the rss

    unsigned int  startcode;                // The address above which program text can run
    unsigned int	endcode;                  // The address below which program text can run
    unsigned int  startstack;               // The address of the start of the stack
    unsigned int  kstkesp;                  // The current value of ESP
    unsigned int  kstkeip;                  // The current value of EIP
//    int           signal;                   // The bitmap of pending signals
//    int           blocked;                  // The bitmap of blocked signals
//    int           sigignore;                // The bitmap of ignored signals
//    int           sigcatch;                 // The bitmap of catched signals
    unsigned int  wchan;                    // (too long)
//    int           sched;                    // scheduler
//    int           sched_priority;           // scheduler priority
  };

  procinfo_t process;
  char path[PATH_MAX] = {0};
  std::sprintf(path, "/proc/%d/stat", pid);

  if(::access(path, R_OK) == posix::error_response)
    return false;

  FILE* file = std::fopen(path, "r");
  if(file == nullptr)
    return false;

  //char state;
  std::fscanf(file, "%d %s %c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u",
              &data.process_id,
              path,
              reinterpret_cast<char*>(&data.state),
              &data.parent_process_id,
              &data.process_group_id,
              &data.session_id,
              &data.tty,
              &process.tpgid,
              &process.flags,
              &process.minflt,
              &process.cminflt,
              &process.majflt,
              &process.cmajflt,
              &process.utime,
              &process.stime,
              &process.cutime,
              &process.cstime,
              &process.counter,
              &data.priority_value,
              &process.timeout,
              &process.itrealvalue,
              &process.starttime,
              &process.vsize,
              &process.rss,
              &process.rlim,
              &process.startcode,
              &process.endcode,
              &process.startstack,
              &process.kstkesp,
              &process.kstkeip,
              &data.signals_pending,
              &data.signals_blocked,
              &data.signals_ignored,
              &data.signals_caught,
              &process.wchan);
  std::fclose(file);

  data.filename.assign(path+1);
  data.filename.pop_back();

  return true;
}
#else

// PDTK
#include <cxxutils/misc_helpers.h>

// *BSD/Darwin
#include <sys/sysctl.h>

bool procstat(pid_t pid, process_t& data) noexcept
{
  struct kinfo_proc info;
  size_t length;
  int request[6] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid, sizeof(struct kinfo_proc), 0 };

  if(sysctl(request, arraylength(request), nullptr, &length, nullptr, 0) != posix::success)
    return false;

  request[5] = (length / sizeof(struct kinfo_proc));

  if(sysctl(request, arraylength(request), &info, &length, nullptr, 0) != posix::success)
    return false;

  // TODO: copy info.ABC to process.XYZ

  return true;
}
#endif
