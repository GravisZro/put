#include "procstat.h"

#if (defined(__APPLE__) && defined(__MACH__)) /* Darwin 7+     */ || \
    defined(__FreeBSD__)                      /* FreeBSD 4.1+  */ || \
    defined(__DragonFly__)                    /* DragonFly BSD */ || \
    defined(__OpenBSD__)                      /* OpenBSD 2.9+  */ || \
    defined(__NetBSD__)                       /* NetBSD 2+     */

// *BSD/Darwin
#include <sys/sysctl.h>

// PDTK
#include <cxxutils/misc_helpers.h>

posix::error_t procstat(pid_t pid, process_state_t& data) noexcept
{
  struct kinfo_proc info;
  posix::size_t length;
  int request[6] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid, sizeof(struct kinfo_proc), 0 };

  if(sysctl(request, arraylength(request), NULL, &length, NULL, 0) != posix::success_response)
    return posix::error_response;

  request[5] = (length / sizeof(struct kinfo_proc));

  if(sysctl(request, arraylength(request), &info, &length, NULL, 0) != posix::success_response)
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

// POSIX
#include <unistd.h>

// POSIX++
#include <cstdio>
#include <cstring>
#include <climits>

// PDTK
#include <cxxutils/mountpoint_helpers.h>

static posix::size_t arg_max = posix::size_t(sysconf(_SC_ARG_MAX));

typedef posix::error_t (*decode_func)(FILE*, process_state_t&);

posix::error_t proc_decode(pid_t pid, const char* subfile, decode_func func, process_state_t& data)
{
  char filename[PATH_MAX] = { 0 };
  std::snprintf(filename, PATH_MAX, "%s/%d/%s", procfs_path, pid, subfile);

  FILE* file = std::fopen(filename, "r");
  if(file == NULL)
    return posix::error_response;

  if(func(file, data) == posix::error_response)
    return posix::error_response;

  return std::fclose(file);
}

posix::error_t proc_exe_symlink(pid_t pid, const char* subfile, process_state_t& data) noexcept
{
  char linkname[PATH_MAX] = { 0 };
  char filename[PATH_MAX] = { 0 };
  std::snprintf(linkname, PATH_MAX, "%s/%d/%s", procfs_path, pid, subfile); // fill buffer

  posix::ssize_t length = ::readlink(linkname, filename, arg_max); // result may contain " (deleted)" (which we don't want)
  if(length != posix::error_response)
  {
    filename[length] = 0; // add string terminator
    char* pos = std::strrchr(filename, ' '); // find last space
    if(pos != NULL && // contains a space (may be part of " (deleted)")
       pos[sizeof(" (deleted)") - 1] == '\0' && // might end with " (deleted)"
       pos == std::strstr(filename, " (deleted)")) // definately ends with " (deleted)"
      *pos = 0; // add string terminator to truncate at " (deleted)"
    data.executable = filename; // copy corrected string
  }
  return posix::success_response;
}

# if defined(__linux__) // Linux
#include <linux/version.h> // for LINUX_VERSION_CODE and KERNEL_VERSION macros
#include <linux/kdev_t.h> // for MAJOR and MINOR macros
#include <inttypes.h> // for fscanf abstracted type macros

posix::error_t proc_stat_decoder(FILE* file, process_state_t& data) noexcept
{
  struct //procinfo_t
  {
//    pid_t         pid;                      // The process id.
    char          name[PATH_MAX];           // The filename of the executable
    char          state;                    // R is running, S is sleeping, D is sleeping in an uninterruptible wait, Z is zombie, T is traced or stopped
    uid_t         euid;                     // effective user id
    gid_t         egid;                     // effective group id
//    pid_t         ppid;                     // The pid of the parent.
//    pid_t         pgrp;                     // The pgrp of the process.
//    pid_t         session;                  // The session id of the process.
    int           tty;                      // The tty the process uses
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
    int           priority;                 // the standard nice value, plus fifteen
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
    int           signal;                   // The bitmap of pending signals
    int           blocked;                  // The bitmap of blocked signals
    int           sigignore;                // The bitmap of ignored signals
    int           sigcatch;                 // The bitmap of catched signals
    unsigned int  wchan;                    // This is the "channel" in which the process is waiting. It is the address of a location in the kernel where the process is sleeping. The corresponding symbolic name can be found in /proc/[pid]/wchan.
    unsigned int  nswap;                    // Number of pages swapped (not maintained)
    unsigned int  cnswap;                   // Cumulative nswap for child processes (not maintained).
    int           exit_signal;              // Signal to be sent to parent when we die.
    int           processor;                // CPU number last executed on.
    unsigned int  rt_priority;              // Real-time scheduling priority, a number in the range 1 to 99 for processes scheduled under a real-time policy, or 0, for non-real-time processes (see sched_setscheduler(2)).
    unsigned int  policy;                   // Scheduling policy (see sched_setscheduler(2)). Decode using the SCHED_* constants in linux/sched.h.
  } process;

#define Sd16 "%" SCNd16 " "
#define Sd32 "%" SCNd32 " "
#define Sd64 "%" SCNd64 " "

#define Su16 "%" SCNu16 " "
#define Su32 "%" SCNu32 " "
#define Su64 "%" SCNu64 " "

  std::fscanf(file,
              Sd32 // pid
              "%s " // comm
              "%c " // state
              Sd32 // ppid
              Sd32 // pgrp
              Sd32 // session
              Sd32 // tty_nr
              Sd32 // tpgid
              Su32 // flags
              Sd32 // minflt
              Su32 // cminflt
              Su32 // majflt
              Su32 // cmajflt
              Su32 // utime
              Su32 // stime
              Sd32 // cutime
              Sd32 // cstime
              Sd32 // priority
              Sd32 // nice
              Su32 // num_threads
              Su32 // itrealvalue
              Su32 // starttime
              Su32 // vsize
              Sd32 // rss
              Su32 // rsslim
              Su32 // startcode
              Su32 // endcode
              Su32 // startstack
              Su32 // kstkesp
              Su32 // kstkeip
              Su32 // signal
              Su32 // blocked
              Su32 // sigignore
              Su32 // sigcatch
              Su32 // wchan
              Su32 // nswap
              Su32 // cnswap
            #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,22)
              Sd32 // exit_signal
            # if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,8)
              Sd32 // processor
            #  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,19)
              Su32 // rt_priority
              Su32 // policy
            #  endif
            # endif
            #endif
              , &data.process_id
              , process.name
              , &process.state
              , &data.parent_process_id
              , &data.process_group_id
              , &data.session_id
              , &process.tty
              , &process.tpgid
              , &process.flags
              , &process.minflt
              , &process.cminflt
              , &process.majflt
              , &process.cmajflt
              , &process.utime
              , &process.stime
              , &process.cutime
              , &process.cstime
              , &process.counter
              , &process.priority
              , &process.timeout
              , &process.itrealvalue
              , &process.starttime
              , &process.vsize
              , &process.rss
              , &process.rlim
              , &process.startcode
              , &process.endcode
              , &process.startstack
              , &process.kstkesp
              , &process.kstkeip
              , &process.signal
              , &process.blocked
              , &process.sigignore
              , &process.sigcatch
              , &process.wchan
              , &process.nswap
              , &process.cnswap
            #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,22)
              , &process.exit_signal
            # if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,8)
              , &process.processor
            #  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,19)
              , &process.rt_priority
              , &process.policy
            #  endif
            # endif
            #endif
              );

  data.tty_device = dev_t(process.tty);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#else
#endif

  switch(process.state)
  {
    case 'R': data.state = Running; break; // Running
    case 'S': data.state = WaitingInterruptable; break; // Sleeping in an interruptible wait
    case 'D': data.state = WaitingUninterruptable; break; // Waiting in uninterruptible disk sleep
    case 'Z': data.state = Zombie; break; // Zombie
    case 'T': data.state = Stopped; break; // Stopped (on a signal) or (before Linux 2.6.33) trace stopped
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    case 'W': data.state = WaitingUninterruptable; break; // Paging (only before Linux 2.6.0)
#else
    case 'X': data.state = Zombie; break; // Dead (from Linux 2.6.0 onward)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
    case 't': data.state = Stopped; break; // Tracing stop (Linux 2.6.33 onward)
#  if LINUX_VERSION_CODE <= KERNEL_VERSION(3,13,0)
    case 'x': data.state = Zombie; break; // Dead (Linux 2.6.33 to 3.13 only)
    case 'K': data.state = WaitingUninterruptable; break; // Wakekill (Linux 2.6.33 to 3.13 only)
    case 'W': data.state = WaitingInterruptable; break; // Waking (Linux 2.6.33 to 3.13 only)
    case 'P': data.state = WaitingInterruptable; break; // Parked (Linux 3.9 to 3.13 only)
#  endif
# endif
#endif
  }

  data.name.assign(process.name + 1);
  data.name.pop_back();
  return posix::success_response;
}

posix::error_t proc_status_decoder(FILE* file, process_state_t& data) noexcept
{
  uint64_t sigpnd, shdpnd;
  std::fscanf(file,
              "\nSigPnd: %" SCNx64
              "\nShdPnd: %" SCNx64
              "\nSigBlk: %" SCNx64
              "\nSigIgn: %" SCNx64
              "\nSigCgt: %" SCNx64,
              &sigpnd,
              &shdpnd,
              reinterpret_cast<uint64_t*>(&data.signals_blocked),
              reinterpret_cast<uint64_t*>(&data.signals_ignored),
              reinterpret_cast<uint64_t*>(&data.signals_caught));
  data.signals_pending = uint32_t(sigpnd);

  return posix::success_response;
}

posix::error_t proc_cmdline_decoder(FILE* file, process_state_t& data) noexcept
{
  char* cmdbuffer = static_cast<char*>(::malloc(arg_max));
  if(cmdbuffer != NULL)
  {
    while(!std::feof(file))
    {
      std::fscanf(file, "%s ", cmdbuffer);
      if(std::strlen(cmdbuffer))
        data.arguments.push_back(cmdbuffer);
    }

    ::free(cmdbuffer);
    return posix::success_response;
  }
  return posix::error_response;
}

# elif defined(__sun) && defined(__SVR4) // Solaris / OpenSolaris / OpenIndiana / illumos

#include <procfs.h>

posix::error_t proc_psinfo_decoder(FILE* file, process_state_t& data) noexcept
{
  psinfo_t info;
  if (fread(&info, sizeof(info), 1, f) > 0)
  {

  }
}



# elif defined(__osf__) || defined(__osf) // Tru64 (OSF/1)
#  error No /proc decoding is implemented in PDTK for Tru64!  Please submit a patch!
# elif defined(__hpux) // HP-UX
#  error No /proc decoding is implemented in PDTK for HP-UX!  Please submit a patch!
# elif defined(_AIX) // IBM AIX

#include <sys/procfs.h>

posix::error_t proc_psinfo_decoder(FILE* file, process_state_t& data) noexcept
{
  psinfo info;
  if (fread(&info, sizeof(info), 1, f) > 0)
  {

  }
}


#  error No /proc decoding is implemented in PDTK for AIX!  Please submit a patch!
# elif defined(BSD)
#  error No /proc decoding is implemented in PDTK for unrecognized BSD derivative!  Please submit a patch!
# else
#  error No /proc decoding is implemented in PDTK for unrecognized UNIX!  Please submit a patch!
# endif

posix::error_t procstat(pid_t pid, process_state_t& data) noexcept
{
  static_assert(sizeof(gid_t) == sizeof(int), "size error");
  static_assert(sizeof(uid_t) == sizeof(int), "size error");
  static_assert(sizeof(pid_t) == sizeof(int), "size error");

  if(procfs_path == nullptr &&
    initialize_paths() == posix::error_response)
    return posix::error_response;

  data.state = Invalid;
  data.arguments.clear();

# if defined(__linux__) // Linux
  proc_decode(pid, "stat", proc_stat_decoder, data);
  proc_decode(pid, "status", proc_status_decoder, data);
  proc_decode(pid, "cmdline", proc_cmdline_decoder, data);
  proc_exe_symlink(pid, "exe", data);

# elif defined(__sun) && defined(__SVR4) // Solaris / OpenSolaris / OpenIndiana / illumos
  proc_decode(pid, "psinfo", proc_psinfo_decoder, data);
  proc_exe_symlink(pid, "path/a.out", data);

# elif defined(_AIX) // IBM AIX
  proc_decode(pid, "psinfo", proc_psinfo_decoder, data);

# elif defined(__osf__) || defined(__osf) // Tru64 (OSF/1)
#  error No /proc decoding is implemented in PDTK for Tru64!  Please submit a patch!

# elif defined(__hpux) // HP-UX
#  error No /proc decoding is implemented in PDTK for HP-UX!  Please submit a patch!

# elif defined(BSD)
#  error No /proc decoding is implemented in PDTK for unrecognized BSD derivative!  Please submit a patch!
# else
#  error No /proc decoding is implemented in PDTK for unrecognized UNIX!  Please submit a patch!
# endif

  if(!data.arguments.empty())
  {
    const std::string& farg = data.arguments.front();
    if(farg.find(data.name) != std::string::npos)
      data.name = farg.substr(farg.rfind(data.name));
  }

  return posix::success_response;
}

#else
#error Unsupported platform! >:(
#endif

