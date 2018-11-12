#include "procstat.h"

// POSIX++
# include <cstring>
# include <climits>

// PUT
#include <specialized/osdetect.h>

template<typename T>
static inline void copy_struct(T& dest, const T& source)
  { std::memcpy(&dest, &source, sizeof(T)); }

bool split_arguments(std::vector<std::string>& argvector, const char* argstr)
{
  std::string str;
  bool in_quote = false;
  str.reserve(NAME_MAX);
  for(const char* pos = argstr; *pos; ++pos)
  {
    if(*pos == '"')
      in_quote ^= true;
    else if(in_quote)
    {
      if(*pos == '\\')
      {
        switch(*(++pos))
        {
          case 'a' : str.push_back('\a'); break;
          case 'b' : str.push_back('\b'); break;
          case 'f' : str.push_back('\f'); break;
          case 'n' : str.push_back('\n'); break;
          case 'r' : str.push_back('\r'); break;
          case 't' : str.push_back('\t'); break;
          case 'v' : str.push_back('\v'); break;
          case '"' : str.push_back('"' ); break;
          case '\\': str.push_back('\\'); break;
          case '\0': --pos; break;
          default: break;
        }
      }
      else
        str.push_back(*pos);
    }
    else if(std::isspace(*pos))
    {
      if(!str.empty())
      {
        argvector.emplace_back(str);
        str.clear();
      }
    }
    else if(std::isgraph(*pos))
      str.push_back(*pos);
  }
  if(!str.empty())
    argvector.emplace_back(str);
  return true;
}

#if defined(__FreeBSD__)  /* FreeBSD  */ || \
    defined(__OpenBSD__)  /* OpenBSD  */ || \
    defined(__NetBSD__)   /* NetBSD   */ || \
    defined(__darwin__)   /* Darwin   */

// *BSD/Darwin
# include <sys/sysctl.h>
# include <sys/user.h>
# include <sys/proc.h>
# include <sys/resource.h>
# include <sys/resourcevar.h>
# include <sys/signalvar.h>

// PUT
# include <cxxutils/misc_helpers.h>

#if defined(__darwin__) /* Darwin XNU 792+ */
struct session {
   int s_count;  /* Ref cnt; pgrps in session. */
   struct proc *s_leader;  /* Session leader. */
   struct vnode *s_ttyvp;  /* Vnode of controlling terminal. */
   struct tty *s_ttyp;  /* Controlling terminal. */
   pid_t s_sid;  /* Session ID */
   char s_login[MAXLOGNAME]; /* Setlogin() name. */
};

struct pgrp {
   LIST_ENTRY(pgrp) pg_hash; /* Hash chain. */
   LIST_HEAD(, proc) pg_members; /* Pointer to pgrp members. */
   struct session *pg_session; /* Pointer to session. */
   pid_t pg_id;   /* Pgrp id. */
   int pg_jobc; /* # procs qualifying pgrp for job control */
};

#elif (defined(__FreeBSD__) && KERNEL_VERSION_CODE < KERNEL_VERSION(8,0,0)) || \
     (defined(__NetBSD__)  && KERNEL_VERSION_CODE < KERNEL_VERSION(1,5,0)) || \
     (defined(__OpenBSD__) && KERNEL_VERSION_CODE < KERNEL_VERSION(3,7,0))
#  define OLD_BSD

struct kinfo_proc {
   struct proc kp_proc;   /* proc structure */
   struct eproc {
      struct proc *e_paddr;  /* address of proc */
      struct session *e_sess; /* session pointer */
      struct pcred e_pcred;  /* process credentials */
      struct ucred e_ucred;  /* current credentials */
      struct vmspace e_vm;  /* address space */
      pid_t e_ppid;   /* parent process id */
      pid_t e_pgid;   /* process group id */
      short e_jobc;   /* job control counter */
      dev_t e_tdev;   /* controlling tty dev */
      pid_t e_tpgid;  /* tty process group id */
      struct session *e_tsess; /* tty session pointer */
#define WMESGLEN 7
      char e_wmesg[WMESGLEN+1]; /* wchan message */
      segsz_t e_xsize;  /* text size */
      short e_xrssize;  /* text rss */
      short e_xccount;  /* text references */
      short e_xswrss;
      long e_flag;
#define EPROC_CTTY 0x01 /* controlling tty vnode active */
#define EPROC_SLEADER 0x02 /* session leader */
      char e_login[MAXLOGNAME]; /* setlogin() name */
      long e_spare[4];
   } kp_eproc;
};
# endif

bool procstat(pid_t pid, process_state_t& data) noexcept
{
  struct kinfo_proc info;
  posix::size_t length;
  int request[6] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid, sizeof(struct kinfo_proc), 0 };

  if(sysctl(request, arraylength(request), NULL, &length, NULL, 0) != posix::success_response)
    return false;

  request[5] = (length / sizeof(struct kinfo_proc));

  if(sysctl(request, arraylength(request), &info, &length, NULL, 0) != posix::success_response)
    return false;

# if defined(OLD_BSD) || defined(__darwin__)
  // Darwin structure documentation
  // kinfo_proc   : https://opensource.apple.com/source/xnu/xnu-123.5/bsd/sys/sysctl.h.auto.html
  // extern_proc  : https://opensource.apple.com/source/xnu/xnu-123.5/bsd/sys/proc.h.auto.html
  // rusage       : https://opensource.apple.com/source/xnu/xnu-123.5/bsd/sys/resource.h.auto.html
  // pstats       : https://opensource.apple.com/source/xnu/xnu-123.5/bsd/sys/resourcevar.h.auto.html
  // sigacts      : https://opensource.apple.com/source/xnu/xnu-123.5/bsd/sys/signalvar.h.auto.html
  // pgrp/session : https://opensource.apple.com/source/xnu/xnu-792/bsd/sys/proc_internal.h.auto.html

//  if(!split_arguments(data.arguments, info.ki_args->ar_args))
//    return false;

  data.user_id            = info.kp_eproc.e_pcred.p_ruid;
  data.group_id           = info.kp_eproc.e_pcred.p_rgid;
  data.process_id         = info.kp_proc.p_pid;
  data.parent_process_id  = info.kp_eproc.e_ppid;
  data.process_group_id   = info.kp_eproc.e_pgid;
  data.session_id         = info.kp_eproc.e_sess->s_sid;
  data.tty_device         = info.kp_eproc.e_tdev;

  data.name               = info.kp_proc.p_comm;
  data.nice_value         = info.kp_proc.p_nice;

#  if defined(__darwin__)
  copy_struct(data.signals_pending, info.kp_proc.p_sigmask  );
  copy_struct(data.signals_ignored, info.kp_proc.p_sigignore);
  copy_struct(data.signals_caught , info.kp_proc.p_sigcatch );

  //copy_struct(data.start_time , info.kp_proc.p_stats->p_start);
  copy_struct(data.user_time  , info.kp_proc.p_ru->ru_utime);
  copy_struct(data.system_time, info.kp_proc.p_ru->ru_stime);
#  else
  copy_struct(data.signals_pending, info.kp_proc.p_sigacts->ps_sigonstack);
//copy_struct(data.signals_blocked, info.kp_proc.p_sigacts->ps_catchmask[???]); // TODO: Figure this part out!
  copy_struct(data.signals_ignored, info.kp_proc.p_sigacts->ps_sigignore );
  copy_struct(data.signals_caught , info.kp_proc.p_sigacts->ps_sigcatch  );

  copy_struct(data.start_time , info.kp_proc.p_stats->p_start);
  copy_struct(data.user_time  , info.kp_proc.p_ru.ru_utime);
  copy_struct(data.system_time, info.kp_proc.p_ru.ru_stime);
#  endif

# else
  // BSD structure documentation
  // kinfo_proc : https://github.com/freebsd/freebsd/blob/master/sys/sys/user.h#L121
  // pargs      : https://github.com/freebsd/freebsd/blob/master/sys/sys/proc.h#L118
  // rusage     : https://github.com/freebsd/freebsd/blob/master/sys/sys/resource.h#L73

  if(!split_arguments(data.arguments, info.ki_args->ar_args)) // pargs
    return false;

  data.user_id            = info.ki_ruid;
  data.group_id           = info.ki_rgid;
  data.process_id         = info.ki_pid;
  data.parent_process_id  = info.ki_ppid;
  data.process_group_id   = info.ki_pgid;
  data.session_id         = info.ki_sid;
  data.tty_device         = info.ki_tdev;

  data.name               = info.ki_comm;
  data.nice_value         = info.ki_nice;

  copy_struct(data.signals_pending, info.ki_siglist  );
  copy_struct(data.signals_blocked, info.ki_sigmask  );
  copy_struct(data.signals_ignored, info.ki_sigignore);
  copy_struct(data.signals_caught , info.ki_sigcatch );


  copy_struct(data.start_time , info.ki_start);
  copy_struct(data.user_time  , info.ki_rusage.ru_utime); // rusage
  copy_struct(data.system_time, info.ki_rusage.ru_stime); // rusage

  switch(info.ki_stat) // TODO: lookup values
  {
    case 'R': data.state = Running; break; // Running
    case 'S': data.state = WaitingInterruptable; break; // Sleeping in an interruptible wait
    case 'D': data.state = WaitingUninterruptable; break; // Waiting in uninterruptible disk sleep
    case 'Z': data.state = Zombie; break; // Zombie
    case 'T': data.state = Stopped; break; // Stopped (on a signal) or (before Linux 2.6.33) trace stopped
  }

# endif

  return true;
}

#elif defined(__QNX__)
// https://users.pja.edu.pl/~jms/qnx/help/watcom/clibref/qnx/qnx_psinfo.html

# include <sys/psinfo.h>
# error No Process state code exists in PUT for QNX!  Please submit a patch!


#elif defined(__unix__) /* Generic UNIX */

// POSIX
# include <unistd.h>

// POSIX++
# include <cstdio>

// PUT
# include <specialized/mountpoints.h>

static posix::size_t arg_max = posix::size_t(sysconf(_SC_ARG_MAX));

typedef bool (*decode_func)(FILE*, process_state_t&);

bool proc_decode(pid_t pid, const char* subfile, decode_func func, process_state_t& data)
{
  char filename[PATH_MAX] = { 0 };
  std::snprintf(filename, PATH_MAX, "%s/%d%c%s", procfs_path, pid,
                subfile == nullptr ? '\0' : '/',
                subfile == nullptr ? "" : subfile);

  FILE* file = std::fopen(filename, "r");
  if(file == NULL)
    return posix::error_response;

  if(!func(file, data))
    return false;

  return std::fclose(file) == posix::error_response;
}

bool proc_exe_symlink(pid_t pid, const char* subfile, process_state_t& data) noexcept
{
  char linkname[PATH_MAX] = { 0 };
  char filename[PATH_MAX] = { 0 };
  std::snprintf(linkname, PATH_MAX, "%s/%d/%s", procfs_path, pid, subfile); // fill buffer

  posix::ssize_t length = ::readlink(linkname, filename, arg_max); // result may contain " (deleted)" (which we don't want)
  if(length == posix::error_response)
    return false;

  filename[length] = 0; // add string terminator
  char* pos = std::strrchr(filename, ' '); // find last space
  if(pos != NULL && // contains a space (may be part of " (deleted)")
     pos[sizeof(" (deleted)") - 1] == '\0' && // might end with " (deleted)"
     pos == std::strstr(filename, " (deleted)")) // definately ends with " (deleted)"
    *pos = 0; // add string terminator to truncate at " (deleted)"
  data.executable = filename; // copy corrected string
  return true;
}

# if defined(__linux__) /* Linux    */

// POSIX
#  include <inttypes.h> // for fscanf macros

bool proc_stat_decoder(FILE* file, process_state_t& data) noexcept
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

  std::fscanf(file,
              "%" SCNd32 " " // pid
              "%s " // comm
              "%c " // state
              "%" SCNd32 " " // ppid
              "%" SCNd32 " " // pgrp
              "%" SCNd32 " " // session
              "%" SCNd64 " " // tty_nr
              "%" SCNd32 " " // tpgid
              "%" SCNu32 " " // flags
              "%" SCNd32 " " // minflt
              "%" SCNu32 " " // cminflt
              "%" SCNu32 " " // majflt
              "%" SCNu32 " " // cmajflt
              "%" SCNu32 " " // utime
              "%" SCNu32 " " // stime
              "%" SCNd32 " " // cutime
              "%" SCNd32 " " // cstime
              "%" SCNd32 " " // priority
              "%" SCNd32 " " // nice
              "%" SCNu32 " " // num_threads
              "%" SCNu32 " " // itrealvalue
              "%" SCNu32 " " // starttime
              "%" SCNu32 " " // vsize
              "%" SCNd32 " " // rss
              "%" SCNu32 " " // rsslim
              "%" SCNu32 " " // startcode
              "%" SCNu32 " " // endcode
              "%" SCNu32 " " // startstack
              "%" SCNu32 " " // kstkesp
              "%" SCNu32 " " // kstkeip
              "%" SCNu32 " " // signal
              "%" SCNu32 " " // blocked
              "%" SCNu32 " " // sigignore
              "%" SCNu32 " " // sigcatch
              "%" SCNu32 " " // wchan
              "%" SCNu32 " " // nswap
              "%" SCNu32 " " // cnswap
            #if KERNEL_VERSION_CODE >= KERNEL_VERSION(2,1,22)
              "%" SCNd32 " " // exit_signal
            # if KERNEL_VERSION_CODE >= KERNEL_VERSION(2,2,8)
              "%" SCNd32 " " // processor
            #  if KERNEL_VERSION_CODE >= KERNEL_VERSION(2,5,19)
              "%" SCNu32 " " // rt_priority
              "%" SCNu32 " " // policy
            #  endif
            # endif
            #endif
              , &data.process_id
              , process.name
              , &process.state
              , &data.parent_process_id
              , &data.process_group_id
              , &data.session_id
              , &data.tty_device
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
            #if KERNEL_VERSION_CODE >= KERNEL_VERSION(2,1,22)
              , &process.exit_signal
            # if KERNEL_VERSION_CODE >= KERNEL_VERSION(2,2,8)
              , &process.processor
            #  if KERNEL_VERSION_CODE >= KERNEL_VERSION(2,5,19)
              , &process.rt_priority
              , &process.policy
            #  endif
            # endif
            #endif
              );

  switch(process.state)
  {
    case 'R': data.state = Running; break; // Running
    case 'S': data.state = WaitingInterruptable; break; // Sleeping in an interruptible wait
    case 'D': data.state = WaitingUninterruptable; break; // Waiting in uninterruptible disk sleep
    case 'Z': data.state = Zombie; break; // Zombie
    case 'T': data.state = Stopped; break; // Stopped (on a signal) or (before Linux 2.6.33) trace stopped
#  if KERNEL_VERSION_CODE < KERNEL_VERSION(2,6,0)
    case 'W': data.state = WaitingUninterruptable; break; // Paging (only before Linux 2.6.0)
#  else
    case 'X': data.state = Zombie; break; // Dead (from Linux 2.6.0 onward)
#   if KERNEL_VERSION_CODE >= KERNEL_VERSION(2,6,33)
    case 't': data.state = Stopped; break; // Tracing stop (Linux 2.6.33 onward)
#    if KERNEL_VERSION_CODE <= KERNEL_VERSION(3,13,0)
    case 'x': data.state = Zombie; break; // Dead (Linux 2.6.33 to 3.13 only)
    case 'K': data.state = WaitingUninterruptable; break; // Wakekill (Linux 2.6.33 to 3.13 only)
    case 'W': data.state = WaitingInterruptable; break; // Waking (Linux 2.6.33 to 3.13 only)
    case 'P': data.state = WaitingInterruptable; break; // Parked (Linux 3.9 to 3.13 only)
#    endif
#   endif
#  endif
  }

  data.name.assign(process.name + 1);
  data.name.pop_back();
  return true;
}

bool proc_status_decoder(FILE* file, process_state_t& data) noexcept
{
  uint64_t shdpnd;
  std::fscanf(file,
              "\nSigPnd: %" SCNx64
              "\nShdPnd: %" SCNx64
              "\nSigBlk: %" SCNx64
              "\nSigIgn: %" SCNx64
              "\nSigCgt: %" SCNx64,
              reinterpret_cast<uint64_t*>(&data.signals_pending),
              &shdpnd,
              reinterpret_cast<uint64_t*>(&data.signals_blocked),
              reinterpret_cast<uint64_t*>(&data.signals_ignored),
              reinterpret_cast<uint64_t*>(&data.signals_caught));
  return true;
}

bool proc_cmdline_decoder(FILE* file, process_state_t& data) noexcept
{
  char* cmdbuffer = static_cast<char*>(::malloc(arg_max));
  bool rval = true;
  if(cmdbuffer == NULL ||
     fread(cmdbuffer, arg_max, 1, file) <= 0 ||
     !split_arguments(data.arguments, cmdbuffer))
    rval = false;
  if(cmdbuffer)
    ::free(cmdbuffer);
  cmdbuffer = nullptr;
  return rval;
}

# elif defined(__solaris__) /* Solaris  */ || \
       defined(__aix__)     /* AIX      */

// POSIX++
#  include <cctype>
#  include <cassert>

#  if defined(__solaris__)
// Solaris
#   include <procfs.h>
#  else
// AIX
#   include <sys/procfs.h>
typdef struct psinfo  psinfo_t;
typdef struct pstatus pstatus_t;
#  endif

bool proc_psinfo_decoder(FILE* file, process_state_t& data) noexcept
{
  psinfo_t info;
  if(fread(&info, sizeof(info), 1, file) <= 0 ||
     !split_arguments(data.arguments, info.pr_psargs))
    return false;

  data.user_id            = info.pr_uid;
  data.group_id           = info.pr_gid;
  data.process_id         = info.pr_pid;
  data.parent_process_id  = info.pr_ppid;
  data.process_group_id   = info.pr_pgid;
  data.session_id         = info.pr_sid;
  data.tty_device         = info.pr_ttydev;
  data.name               = info.pr_fname;
  return true;
}

bool proc_status_decoder(FILE* file, process_state_t& data) noexcept
{
  pstatus_t status;
  if(fread(&status, sizeof(status), 1, file) <= 0)
    return false;

  data.signals_pending = status.pr_lwp.pr_lwppend;
  data.signals_blocked = status.pr_lwp.pr_lwphold;
  data.user_time       = status.pr_utime;
  data.system_time     = status.pr_stime;

  return true;
}

bool proc_sigact_decoder(FILE* file, process_state_t& data) noexcept
{
  return true; // TODO!
}

# elif defined(__hpux__)  /* HP-UX    */

// HP-UX
#include <sys/param.h>
#include <sys/pstat.h>

bool proc_hpux_decode(pid_t pid, decode_func func, process_state_t& data)
{
  pst_status status;
  if(pstat_getproc(&statux, sizeof(pst_status), 0, pid) == posix::error_response ||
     !split_arguments(data.arguments, status.pst_cmd))
    return false;

/*
  euid = pi->pst_euid;
  egid = pi->pst_egid;

  context_switches = pi->pst_nvcsw + pi->pst_nivcsw;
  voluntary_context_switches = pi->pst_nvcsw;
  involuntary_context_switches = pi->pst_nivcsw;
  proc_size = (pi->pst_dsize + pi->pst_tsize + pi->pst_ssize) * sys_page_size;
  proc_resident = pi->pst_rssize * sys_page_size;
  start_time = pi->pst_start;
  time_spent = pi->pst_time;
  cpu_percent = (pi->pst_pctcpu * 100.0) / 0x8000;
*/
  // TODO: Missing time, signal sets and memory stats

  data.name               = status.pst_ucomm;
  data.user_id            = status.pst_uid;
  data.group_id           = status.pst_gid;
  data.process_id         = status.pst_pid;
  data.parent_process_id  = status.pst_ppid;
  data.process_group_id   = status.pst_pgrp;
  data.session_id         = status.pst_sid; // TODO: find out if this exist
  data.nice_value         = status.pst_n;

  switch(status.pst_stat)
  {
    case PS_SLEEP:  data.state = WaitingInterruptable; break;
    case PS_IDLE:   data.state = WaitingUninterruptable; break; // TODO: find out what PS_IDLE really is
    case PS_RUN:    data.state = Running; break;
    case PS_STOP:   data.state = Stopped; break;
    case PS_ZOMBIE: data.state = Zombie;  break;
    default:
    case PS_OTHER:  data.state = Invalid; break;
  }

}

# elif defined(__tru64__) /* Tru64      */ || \
       defined(__irix__)  /* IRIX       */

// TODO: Missing time and memory stats

// Tru64/IRIX
#  include <sys/procfs.h>

bool proc_pid_decoder(FILE* file, process_state_t& data) noexcept
{
  prpsinfo_t info;
  prstatus_t status;
  prusage_t  usage;
  posix::fd_t fd = ::fileno(file);

  if(fd == posix::error_response ||
     posix::ioctl(fd, PIOCPSINFO, &info  ) == posix::error_response ||
     posix::ioctl(fd, PIOCSTATUS, &status) == posix::error_response ||
     posix::ioctl(fd, PIOCUSAGE,  &usage ) == posix::error_response)
     !split_arguments(data.arguments, info.pr_psargs))
    return false;

  data.user_id            = info.pr_uid;
  data.group_id           = info.pr_gid;
  data.process_id         = info.pr_pid;
  data.parent_process_id  = info.pr_ppid;
  data.process_group_id   = info.pr_pgid;
  data.session_id         = info.pr_sid;
  data.tty_device         = info.pr_ttydev;
  data.nice_value         = info.pr_nice;
  data.tty_device         = info.pr_ttydev;
  data.name               = info.pr_fname;
  data.signals_pending    = status.pr_sigpend;
  data.signals_blocked    = status.pr_sighold;


  copy_struct(data.signals_pending, status.pr_sigpend);
  copy_struct(data.signals_blocked, status.pr_sighold);
//  copy_struct(data.signals_ignored, ); // TODO: determine if these exist
//  copy_struct(data.signals_caught , );

  copy_struct(data.start_time , usage.pu_starttime);
  copy_struct(data.user_time  , usage.pu_utime);
  copy_struct(data.system_time, usage.pu_stime);

  switch(info.pr_sname) // TODO: lookup values
  {
    case 'R': data.state = Running; break; // Running
    case 'S': data.state = WaitingInterruptable; break; // Sleeping in an interruptible wait
    case 'D': data.state = WaitingUninterruptable; break; // Waiting in uninterruptible disk sleep
    case 'Z': data.state = Zombie; break; // Zombie
    case 'T': data.state = Stopped; break; // Stopped (on a signal) or (before Linux 2.6.33) trace stopped
  }

  if(info.pr_zomb)
    data.state = Zombie;

  return true;
}

# endif

inline void clear_state(process_state_t& data) noexcept
{
  data.name.clear();
  data.executable.clear();
  data.arguments.clear();
  data.state              = Invalid;
  data.user_id            = uid_t(-1);
  data.group_id           = gid_t(-1);
  data.process_id         = 0;
  data.parent_process_id  = 0;
  data.process_group_id   = 0;
  data.session_id         = 0;
  data.tty_device         = 0;
  data.priority_value     = 0;
  data.nice_value         = 0;
  std::memset(&data.signals_pending, 0, sizeof(sigset_t));
  std::memset(&data.signals_blocked, 0, sizeof(sigset_t));
  std::memset(&data.signals_ignored, 0, sizeof(sigset_t));
  std::memset(&data.signals_caught , 0, sizeof(sigset_t));
  std::memset(&data.user_time      , 0, sizeof(timeval ));
  std::memset(&data.system_time    , 0, sizeof(timeval ));
}

bool procstat(pid_t pid, process_state_t& data) noexcept
{
  clear_state(data);
  if(procfs_path == nullptr &&
    initialize_paths())
    return posix::error_response;

# if defined(__linux__) // Linux
  if(!proc_decode(pid, "stat"    , proc_stat_decoder   , data) ||
     !proc_decode(pid, "status"  , proc_status_decoder , data) ||
     !proc_decode(pid, "cmdline" , proc_cmdline_decoder, data) ||
     !proc_exe_symlink(pid, "exe", data))
    return false;

# elif defined(__solaris__) /* Solaris  */ || \
       defined(__aix__)     /* AIX      */
  if(!proc_decode(pid, "psinfo", proc_psinfo_decoder, data) ||
     !proc_decode(pid, "status", proc_status_decoder, data) ||
     !proc_decode(pid, "sigact", proc_sigact_decoder, data) ||
     !proc_exe_symlink(pid, "path/a.out", data))
    return false;

# elif defined(__tru64__) /* Tru64      */ || \
       defined(__irix__)  /* IRIX       */
  if(!proc_decode(pid, nullptr, proc_pid_decoder, data))
    return false;

# elif defined(__hpux__) /* HP-UX */
  if(!proc_hpux_decode(pid, data))
    return false;
# else
#  error No /proc decoding is implemented in PUT for unrecognized UNIX!  Please submit a patch!
# endif

  if(!data.arguments.empty())
  {
    const std::string& farg = data.arguments.front();
    if(farg.find(data.name) != std::string::npos)
      data.name = farg.substr(farg.rfind(data.name));
  }

  return true;
}

#else
# error Unsupported platform! >:(
#endif

