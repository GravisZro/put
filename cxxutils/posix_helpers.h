#ifndef POSIX_HELPERS_H
#define POSIX_HELPERS_H

// POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

// POSIX++
#include <cstring>
#include <csignal>

// PDTK
#include "error_helpers.h"

#ifndef SIGPOLL
# ifdef SIGIO
#  define SIGPOLL SIGIO
# else
#  error Neither SIGPOLL nor SIGIO is defined
# endif
#endif

static_assert(sizeof(::size_t) == sizeof(std::size_t), "STL's size_t doesn't match the C standard!");

namespace posix
{
  using ::size_t;
  using ::ssize_t;

  typedef int fd_t;
  static const fd_t invalid_descriptor = -1;

  template<typename RType, typename... ArgTypes>
  using function = RType(*)(ArgTypes...);

  template<typename RType, typename... ArgTypes>
  static inline RType ignore_interruption(function<RType, ArgTypes...> func, ArgTypes... args) noexcept
  {
    RType rval = error_response;
    do {
      rval = func(args...);
    } while(rval == error_response && errno == std::errc::interrupted);
    return rval;
  }

  template<typename RType, typename... ArgTypes>
  static inline RType* ignore_interruption(function<RType*, ArgTypes...> func, ArgTypes... args) noexcept
  {
    RType* rval = nullptr;
    do {
      rval = func(args...);
    } while(rval == nullptr && errno == std::errc::interrupted);
    return rval;
  }

// POSIX wrappers
  static inline passwd* getpwuid(uid_t uid) noexcept
    { return ignore_interruption<passwd, uid_t>(::getpwuid, uid); }

  static inline group* getgrgid(gid_t gid) noexcept
    { return ignore_interruption<group, gid_t>(::getgrgid, gid); }

  static inline passwd* getpwnam(const char* username) noexcept
    { return ignore_interruption<passwd, const char*>(::getpwnam, username); }

  static inline group* getgrnam(const char* groupname) noexcept
    { return ignore_interruption<group, const char*>(::getgrnam, groupname); }

// shortcuts
  static inline const char* getusername(uid_t uid) noexcept
  {
    passwd* rval = posix::getpwuid(uid);
    return rval == nullptr ? nullptr : rval->pw_name;
  }

  static inline const char* getgroupname(gid_t gid) noexcept
  {
    group* rval = posix::getgrgid(gid);
    return rval == nullptr ? nullptr : rval->gr_name;
  }

  static inline uid_t getuserid(const char* name) noexcept
  {
    passwd* rval = posix::getpwnam(name);
    return rval == nullptr ? error_response : rval->pw_uid;
  }

  static inline uid_t getgroupid(const char* name) noexcept
  {
    group* rval = posix::getgrnam(name);
    return rval == nullptr ? error_response : rval->gr_gid;
  }

  static inline bool useringroup(const char* groupname, const char* username)
  {
    if(username == nullptr)
      return false;
    group* grp = posix::getgrnam(groupname);
    if(grp == nullptr)
      return false;
    for(char** member = grp->gr_mem; *member != nullptr; ++member)
      if(!std::strcmp(*member, username))
        return true;
    return false;
  }

// longcuts
  namespace signal
  {
    enum EId : int
    {
      Abort                   = SIGABRT,    // Process abort signal.
      Timer                   = SIGALRM,    // Alarm clock.
      MemoryBusError          = SIGBUS,     // Access to an undefined portion of a memory object.
      ChildStatusChanged      = SIGCHLD,    // Child process terminated, stopped, or continued.
      Resume                  = SIGCONT,    // Continue executing, if stopped.
      FloatingPointException  = SIGFPE,     // Erroneous arithmetic operation.
      Hangup                  = SIGHUP,     // Hangup.
      IllegalInstruction      = SIGILL,     // Illegal instruction.
      Interrupt               = SIGINT,     // Terminal interrupt signal.
      Kill                    = SIGKILL,    // Kill (cannot be caught or ignored).
      BrokenPipe              = SIGPIPE,    // Write on a pipe with no one to read it.
      Quit                    = SIGQUIT,    // Terminal quit signal.
      SegmentationViolation   = SIGSEGV,    // Invalid memory reference.
      Stop                    = SIGSTOP,    // Stop executing (cannot be caught or ignored).
      Terminate               = SIGTERM,    // Termination signal.
      KeyboardStop            = SIGTSTP,    // Terminal stop signal.
      TTYRead                 = SIGTTIN,    // Background process attempting read.
      TTYWrite                = SIGTTOU,    // Background process attempting write.
      UserSignal1             = SIGUSR1,    // User-defined signal 1.
      UserSignal2             = SIGUSR2,    // User-defined signal 2.
      Poll                    = SIGPOLL,    // Pollable event.
      ProfilingTimer          = SIGPROF,    // Profiling timer expired.
      BadSystemCall           = SIGSYS,     // Bad system call.
      TraceTrap               = SIGTRAP,    // Trace/breakpoint trap.
      Urgent                  = SIGURG,     // High bandwidth data is available at a socket.
      VirtualTimer            = SIGVTALRM,  // Virtual timer expired.
      LimitExceededCPU        = SIGXCPU,    // CPU time limit exceeded.
      LimitExceededFile       = SIGXFSZ,    // File size limit exceeded.
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
      ChildTerminated                   = CLD_KILLED,
      ChildTerminatedAndDumped          = CLD_DUMPED,
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
      { return std::raise(id) == success_response; }

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

// POSIX wrappers
  static inline bool chmod(const char* path, mode_t mode) noexcept
    { return ignore_interruption(::chmod, path, mode) != error_response; }

  static inline bool chown(const char* path, uid_t owner, gid_t group) noexcept
    { return ignore_interruption(::chown, path, owner, group) != error_response; }


/*
  // POSIX wrappers
  static inline FILE* fopen(const char* filename, const char* mode) noexcept
    { return ignore_interruption(::fopen, filename, mode); }

  static inline bool fclose(FILE* stream) noexcept
    { return ignore_interruption(::fclose, stream); }

  static inline bool fgets(char* s, int n, FILE* stream) noexcept
    { return ignore_interruption(::fgets, s, n, stream); }

  static inline int fgetc(FILE* stream) noexcept
    { return ignore_interruption(::fgetc, stream); }
*/
}

#endif // POSIX_HELPERS_H
