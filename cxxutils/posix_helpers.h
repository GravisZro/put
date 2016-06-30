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
#include <unistd.h>

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
  static inline RType ignore_interruption(function<RType, ArgTypes...> func, ArgTypes... args) noexcept
#else
  template<typename RType, typename... ArgTypes>
  static inline RType ignore_interruption(RType(*func)(ArgTypes...), ArgTypes... args) noexcept
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
  static inline RType* ignore_interruption(function<RType*, ArgTypes...> func, ArgTypes... args) noexcept
#else
  template<typename RType, typename... ArgTypes>
  static inline RType* ignore_interruption(RType*(*func)(ArgTypes...), ArgTypes... args) noexcept
#endif
  {
    RType* rval = nullptr;
    do {
      rval = func(args...);
    } while(rval == nullptr && errno == std::errc::interrupted);
    return rval;
  }

#ifdef __clang__
#define __uid_t uid_t
#define __gid_t gid_t
#endif

// POSIX wrappers
  static inline passwd* getpwuid(uid_t uid) noexcept
    { return ignore_interruption<passwd, __uid_t>(::getpwuid, uid); }

  static inline group* getgrgid(gid_t gid) noexcept
    { return ignore_interruption<group, __gid_t>(::getgrgid, gid); }

// shortcuts
  static inline std::string getusername(uid_t uid) noexcept
  {
    passwd* rval = posix::getpwuid(uid);
    return rval == nullptr ? "" : rval->pw_name;
  }

  static inline std::string getgroupname(gid_t gid) noexcept
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

    static inline bool raise(EId id) noexcept
      { return ::raise(id) == success_response; }

    static inline bool send(pid_t pid, EId id, int value = 0) noexcept
      { return ::sigqueue(pid, id, {value}) == success_response; }
  }

// POSIX wrappers
  static inline bool dup2(int a, int b) noexcept
    { return ignore_interruption(::dup2, a, b) != error_response; }

  static inline bool close(fd_t fd) noexcept
    { return ignore_interruption(::close, fd) != error_response; }

  static inline ssize_t write(fd_t fd, const void* buffer, size_t length) noexcept
    { return ignore_interruption(::write, fd, buffer, length); }

  static inline ssize_t read(fd_t fd, void* buffer, size_t length) noexcept
    { return ignore_interruption(::read, fd, buffer, length); }
}

#endif // POSIX_HELPERS_H
