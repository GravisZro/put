#include "procstat.h"

#if defined(__linux__) // Linux
// POSIX++
#include <cstdio>
#include <cstring>

// Linux
#include <linux/limits.h>
#ifndef ARG_MAX
#define ARG_MAX 131072
#endif

posix::error_t procstat(pid_t pid, process_state_t& data) noexcept
{
  static_assert(sizeof(gid_t) == sizeof(int), "size error");
  static_assert(sizeof(uid_t) == sizeof(int), "size error");
  static_assert(sizeof(pid_t) == sizeof(int), "size error");

  data.state = Invalid;
  data.arguments.clear();

  {
    struct //procinfo_t
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
      unsigned int  flags;                    // The flags of the process.
      unsigned int  minflt;                   // The number of minor faults
      unsigned int  cminflt;                  // The number of minor faults with childs
      unsigned int  majflt;                   // The number of major faults
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
      unsigned int  endcode;                  // The address below which program text can run
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
    } process;

    char filename[FILENAME_MAX] = { 0 };
    std::sprintf(filename, "/proc/%d/stat", pid);

    FILE* file = std::fopen(filename, "r");
    if(file == nullptr)
      return posix::error_response;

    //char state;
    std::fscanf(file, "%d %s %c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u",
                &data.process_id,
                filename,
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

    data.name.assign(filename+1);
    data.name.pop_back();
  }

  {
    char linkname[FILENAME_MAX] = { 0 };
    char filename[FILENAME_MAX] = { 0 };
    std::sprintf(linkname, "/proc/%d/exe", pid); // fill buffer

    posix::ssize_t length = ::readlink(linkname, filename, ARG_MAX); // result may contain " (deleted)" (which we don't want)
    if(length != posix::error_response)
    {
      filename[length] = 0; // add string terminator
      char* pos = std::strrchr(filename, ' '); // find last space
      if(pos != nullptr && // contains a space (may be part of " (deleted)")
         pos[sizeof(" (deleted)") - 1] == '\0' && // might end with " (deleted)"
         pos == std::strstr(filename, " (deleted)")) // definately ends with " (deleted)"
        *pos = 0; // add string terminator to truncate at " (deleted)"
      data.executable = filename; // copy corrected string
    }
  }

  {
    char cmdbuffer[ARG_MAX] = { 0 };
    char filename[FILENAME_MAX] = { 0 };
    std::sprintf(filename, "/proc/%d/cmdline", pid); // fill buffer

    FILE* file = std::fopen(filename, "r");
    if(file == nullptr)
      return posix::error_response;

    while(!std::feof(file))
    {
      std::fscanf(file, "%s ", cmdbuffer);
      if(std::strlen(cmdbuffer))
        data.arguments.push_back(cmdbuffer);
    }

    std::fclose(file);
  }

  if(!data.arguments.empty())
  {
    const std::string& farg = data.arguments.front();
    if(farg.find(data.name) != std::string::npos)
      data.name = farg.substr(farg.rfind(data.name));
  }

  return posix::success_response;
}

#elif defined(BSD) || defined(__APPLE__) // *BSD/Darwin

// *BSD/Darwin
#include <sys/sysctl.h>

// PDTK
#include <cxxutils/misc_helpers.h>

posix::error_t procstat(pid_t pid, process_state_t& data) noexcept
{
  struct kinfo_proc info;
  posix::size_t length;
  int request[6] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid, sizeof(struct kinfo_proc), 0 };

  if(sysctl(request, arraylength(request), nullptr, &length, nullptr, 0) != posix::success_response)
    return posix::error_response;

  request[5] = (length / sizeof(struct kinfo_proc));

  if(sysctl(request, arraylength(request), &info, &length, nullptr, 0) != posix::success_response)
    return posix::error_response;

#if defined(__APPLE__)
  // Darwin structure documentation
  // kinfo_proc   : https://opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/sys/sysctl.h.auto.html
  // extern_proc  : https://opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/sys/proc.h.auto.html

  data.process_id        = info.kp_proc.p_pid;
  data.parent_process_id = info.kp_eproc.e_ppid;
  data.process_group_id  = info.kp_eproc.e_pgid;
//  data.session_id        = info.kp_eproc.e_sess->;
//  data.priority_value    = info.kp_eproc
//  data.state =
//  data.name =

#else
  // BSD structure documentation
  // kinfo_proc : http://fxr.watson.org/fxr/source/sys/user.h#L119
  // proc       : http://fxr.watson.org/fxr/source/sys/proc.h#L522
  // priority   : http://fxr.watson.org/fxr/source/sys/priority.h#L126
  // pargs      : http://fxr.watson.org/fxr/source/sys/proc.h#L110

  data.process_id        = info.ki_pid;
  data.parent_process_id = info.ki_ppid;
  data.process_group_id  = info.ki_pgid;
  data.session_id        = info.ki_sid;
  data.priority_value    = info.ki_pri.pri_user;
//  data.state             = ;
//  data.name              = ;
//  data.arguments
#endif

  return posix::success_response;
}

#elif defined(__unix__)

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif

