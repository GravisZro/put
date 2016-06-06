#ifndef POSIX_HELPERS_H
#define POSIX_HELPERS_H

// STL
#include <string>
#include <cerrno>
#include <set>

// POSIX
#include <pwd.h>
#include <grp.h>
#include <signal.h>

// PDTK
#include "error_helpers.h"

namespace posix
{
  typedef int fd_t;
  static const fd_t invalid_descriptor = -1;

#ifndef __clang__
  template<typename RType, typename... ArgTypes>
  using function = RType(*)(ArgTypes...);

  template<typename RType, typename... ArgTypes>
  static inline RType ignore_interruption(function<RType, ArgTypes...> func, ArgTypes... args)
#else
  template<typename RType, typename... ArgTypes>
  static inline RType ignore_interruption(RType(*func)(ArgTypes...), ArgTypes... args)
#endif
  {
    RType rval = error_response;
    do {
      rval = func(args...);
    } while(rval == error_response && errno == std::errc::interrupted);
    return rval;
  }

#ifndef __clang__
  template<typename RType, typename... ArgTypes>
  static inline RType* ignore_interruption(function<RType*, ArgTypes...> func, ArgTypes... args)
#else
  template<typename RType, typename... ArgTypes>
  static inline RType* ignore_interruption(RType*(*func)(ArgTypes...), ArgTypes... args)
#endif
  {
    RType* rval = nullptr;
    do {
      rval = func(args...);
    } while(rval == nullptr && errno == std::errc::interrupted);
    return rval;
  }

// POSIX wrappers
  static inline passwd* getpwuid(uid_t uid)
    { return ignore_interruption<passwd, __uid_t>(::getpwuid, uid); }

  static inline group* getgrgid(gid_t gid)
    { return ignore_interruption<group, __gid_t>(::getgrgid, gid); }

// shortcuts
  static inline std::string getusername(uid_t uid)
  {
    passwd* rval = posix::getpwuid(uid);
    return rval == nullptr ? "" : rval->pw_name;
  }

  static inline std::string getgroupname(gid_t gid)
  {
    group* rval = posix::getgrgid(gid);
    return rval == nullptr ? "" : rval->gr_name;
  }

  namespace signal
  {
    enum EId : int
    {
      Hangup                  = SIGHUP,
      Interrupt               = SIGINT,
      Quit                    = SIGQUIT,
      IllegalInstruction      = SIGILL,
      TraceTrap               = SIGTRAP,
      Abort                   = SIGABRT,
      BusError                = SIGBUS,
      FloatingPointException  = SIGFPE,
      Kill                    = SIGKILL,
      UserSignal1             = SIGUSR1,
      SegmentationViolation   = SIGSEGV,
      UserSignal2             = SIGUSR2,
      BrokenPipe              = SIGPIPE,
      Timer                   = SIGALRM,
      Terminate               = SIGTERM,
      StackFault              = SIGSTKFLT,
      ChildStatusChanged      = SIGCHLD,
      Resume                  = SIGCONT,
      Stop                    = SIGSTOP,
      KeyboardStop            = SIGTSTP,
      TTYRead                 = SIGTTIN,
      TTYWrite                = SIGTTOU,
      Urgent                  = SIGURG,
      LimitExceededCPU        = SIGXCPU,
      LimitExceededFile       = SIGXFSZ,
      VirtualTimer            = SIGVTALRM,
      ProfilingTimer          = SIGPROF,
      WindowSizeUpdate        = SIGWINCH,
      Poll                    = SIGPOLL,
      PowerFailure            = SIGPWR,
      BadSystemCall           = SIGSYS,
    };

    enum ECode : int
    {
      // SIGILL
      IllegalOpcode                     = ILL_ILLOPC,
      IllegalOperand                    = ILL_ILLOPN,
      IllegalAddressingMode             = ILL_ILLADR,
      IllegalTrap                       = ILL_ILLTRP,
      PrivilegedOpcode                  = ILL_PRVOPC,
      PrivilegedRegister                = ILL_PRVREG,
      CoprocessorError                  = ILL_COPROC,
      InternalStackError                = ILL_BADSTK,

      // SIGFPE
      IntegerDivideByZero               = FPE_INTDIV,
      IntegerOverflow                   = FPE_INTOVF,
      FloatingPointDivideByZero         = FPE_FLTDIV,
      FloatingPointOverflow             = FPE_FLTOVF,
      FloatingPointInderflow            = FPE_FLTUND,
      FloatingPointInexactResult        = FPE_FLTRES,
      FloatingPointInvalidOperation     = FPE_FLTINV,
      SubscriptOutOfRange               = FPE_FLTSUB,

      // SIGSEGV
      AddressNotMappedToObject          = SEGV_MAPERR,
      InvalidPermissionsForMappedObject = SEGV_ACCERR,

      // SIGBUS
      InvalidAddressAlignment           = BUS_ADRALN,
      NonExistantPhysicalAddress        = BUS_ADRERR,
      ObjectSpecificHardwareError       = BUS_OBJERR,

      // SIGTRAP
      ProcessBreakpoint                 = TRAP_BRKPT,
      ProcessTraceTrap                  = TRAP_TRACE,

      // SIGCHLD
      ChildHasExited                    = CLD_EXITED,
      ChildWasKilled                    = CLD_KILLED,
      ChildTerminatedAbnormally         = CLD_DUMPED,
      TracedChildHasTrapped             = CLD_TRAPPED,
      ChildHasStopped                   = CLD_STOPPED,
      StoppedChildHasContinued          = CLD_CONTINUED,

      // SIGPOLL
      DataInputAvailable                = POLL_IN,
      OutputBuffersAvailable            = POLL_OUT,
      InputMessageAvailable             = POLL_MSG,
      IOError                           = POLL_ERR,
      HighPriorityInputAvailable        = POLL_PRI,
      DeviceDisconnected                = POLL_HUP,
    };

    static inline bool raise(EId id)
      { return ::raise(id) == success_response; }

    static inline bool send(pid_t pid, EId id, int value = 0)
      { return ::sigqueue(pid, id, value) == success_response; }
  }
}

#include <spawn.h>

struct PosixSpawnFileActions : posix_spawn_file_actions_t
{
  PosixSpawnFileActions(void) { errno = ::posix_spawn_file_actions_init   (this); }
 ~PosixSpawnFileActions(void) { errno = ::posix_spawn_file_actions_destroy(this); }

  operator const posix_spawn_file_actions_t*(void) const { return this; }

  int addClose(posix::fd_t fd) { return ::posix_spawn_file_actions_addclose(this, fd); }
  int addOpen(posix::fd_t fd, const char* path, int oflag, mode_t mode) { return ::posix_spawn_file_actions_addopen(this, fd, path, oflag, mode); }
  int addDup2(posix::fd_t fd, posix::fd_t newfd) { return ::posix_spawn_file_actions_adddup2(this, fd, newfd); }
};

struct PosixSpawnAttr : posix_spawnattr_t
{
  PosixSpawnAttr(void) { errno = ::posix_spawnattr_init   (this); }
 ~PosixSpawnAttr(void) { errno = ::posix_spawnattr_destroy(this); }

  operator const posix_spawnattr_t*(void) const { return this; }

  int getSigDefault (sigset_t&    set   ) const { return ::posix_spawnattr_getsigdefault  (this, &set   ); }
  int getFlags      (short&       flags ) const { return ::posix_spawnattr_getflags       (this, &flags ); }
  int getPgroup     (pid_t&       pid   ) const { return ::posix_spawnattr_getpgroup      (this, &pid   ); }
  int getSchedParam (sched_param& param ) const { return ::posix_spawnattr_getschedparam  (this, &param ); }
  int getSchedPolicy(int&         policy) const { return ::posix_spawnattr_getschedpolicy (this, &policy); }
  int getSigMask    (sigset_t&    mask  ) const { return ::posix_spawnattr_getsigmask     (this, &mask  ); }

  int setSigDefault (const sigset_t&    set   ) { return ::posix_spawnattr_setsigdefault  (this, &set   ); }
  int setFlags      (const short        flags ) { return ::posix_spawnattr_setflags       (this, flags  ); }
  int setPgroup     (const pid_t        pid   ) { return ::posix_spawnattr_setpgroup      (this, pid    ); }
  int setSchedParam (const sched_param& param ) { return ::posix_spawnattr_setschedparam  (this, &param ); }
  int setSchedPolicy(const int          policy) { return ::posix_spawnattr_setschedpolicy (this, policy ); }
  int setSigMask    (const sigset_t&    mask  ) { return ::posix_spawnattr_setsigmask     (this, &mask  ); }
};


#endif // POSIX_HELPERS_H
