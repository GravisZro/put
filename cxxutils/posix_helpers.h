#ifndef POSIX_HELPERS_H
#define POSIX_HELPERS_H

// POSIX
#include <sys/types.h>  // system specific types
#include <sys/stat.h>   // stat stuff
#include <sys/ioctl.h>  // ioctl() stuff
#include <sys/wait.h>
#include <pwd.h>    // passwd stuff
#include <grp.h>    // group stuff
#include <unistd.h> // LOTS
#include <fcntl.h>  // fcntl() stuff
#include <string.h> // for useringroup()
#include <signal.h> // signal functions
#include <stdio.h>  // file descriptor I/O
#include <stdint.h> // need standard types
#include <stdlib.h> // exit functions
#include <limits.h> // system limits
#include <ctype.h>  // char type detection

// PUT
#include "error_helpers.h"
#include "signal_helpers.h"

static_assert(sizeof(::size_t) == sizeof(::off_t), "size_t not the same is as off_t!");
static_assert(sizeof(::size_t) == sizeof(::ssize_t), "size_t not the same is as ssize_t!");

namespace posix
{
  using ::size_t;
  using ::ssize_t;
  using ::off_t;
  using ::FILE;

  // stdlib.h
  using ::signal;
  using ::exit;
#if (_XOPEN_SOURCE) >= 400
  using ::atexit;
#endif
  using ::malloc;
  using ::free;

  // ctype.h
  using ::isspace;
  using ::isdigit;
  using ::isgraph;

  // string.h
  using ::memset;
  using ::memchr;
  using ::memcmp;
  using ::memcpy;
#if (_XOPEN_SOURCE) >= 400
  using ::memmove;
#endif

#if (_XOPEN_SOURCE) >= 300
  using ::strerror;
#endif
  using ::strchr;
  using ::strrchr;
  using ::strlen;
#if (_XOPEN_SOURCE) >= 700
  using ::strnlen;
#endif
  using ::strstr;

  using ::strcat;
  using ::strncat;
  using ::strcmp;
  using ::strncmp;
  using ::strcpy;
  using ::strncpy;

  using ::atoi;
  using ::atol;
  using ::strtod;
  using ::strtol;
#if (_XOPEN_SOURCE) >= 600
  using ::atoll;
  using ::strtof;
  using ::strtold;
  using ::strtoll;
#endif
#if (_XOPEN_SOURCE) >= 400
  using ::strtoul;
  using ::strtoull;
#endif

  // stdio.h
  using ::getline;
  using ::getdelim;
  using ::printf;
  using ::dprintf;
  using ::sprintf;
#if (_XOPEN_SOURCE) >= 500
  using ::snprintf;
#endif
  using ::fprintf;
  using ::scanf;
  using ::sscanf;
  using ::fscanf;
  using ::ftell;
  using ::fflush;
  using ::fileno;
  using ::rewind;

  // unistd.h
  using ::access;
#if (_XOPEN_SOURCE) >= 700
  using ::faccessat;
#endif
  using ::alarm;
  using ::chdir;
  // ::chown EINTR
  // ::close EINTR

  // ::confstr XSI
  // ::crypt USE EXTERNAL
  // ::_exit DO NO USE
  // ::encrypt USE EXTERNAL

  using ::execl;
  using ::execle;
  using ::execlp;
  using ::execv;
  using ::execve;
  using ::execvp;
#if (_XOPEN_SOURCE) >= 700
  using ::fexecve;
#endif
  using ::getpid;


  typedef int fd_t;
  static const fd_t invalid_descriptor = error_response;

#if (_XOPEN_SOURCE) >= 420
  static inline bool fchdir(fd_t fd) noexcept
    { return ignore_interruption<int, fd_t>(::fchdir, fd) != error_response; }
#endif

  static inline bool setuid(uid_t uid) noexcept
    { return ignore_interruption<int, uid_t>(::setuid, uid) != error_response; }

  static inline bool setgid(gid_t gid) noexcept
    { return ignore_interruption<int, gid_t>(::setgid, gid) != error_response; }

  using ::getuid;
  using ::getgid;

#if (_XOPEN_SOURCE) >= 600
  static inline bool seteuid(uid_t uid) noexcept
    { return ignore_interruption<int, uid_t>(::seteuid, uid) != error_response; }

  static inline bool setegid(gid_t gid) noexcept
    { return ignore_interruption<int, gid_t>(::setegid, gid) != error_response; }
#endif

  using ::geteuid;
  using ::getegid;

  static inline bool pipe(fd_t fildes[2]) noexcept
    { return ignore_interruption<int, int*>(::pipe, fildes) != error_response; }

  using ::dup;
  //using ::dup2; EINTR

  using ::fork;
#if (_XOPEN_SOURCE) >= 500 && defined(_XOPEN_SOURCE_EXTENDED)
  static inline bool sigqueue(pid_t pid, int signo, union sigval value) noexcept
    { return ignore_interruption<int, pid_t, int, union sigval>(::sigqueue, pid, signo, value) != error_response; }
#endif

  static inline bool kill(pid_t pid, int signo) noexcept
    { return ignore_interruption<int, pid_t, int>(::kill, pid, signo) != error_response; }

#if (_XOPEN_SOURCE) >= 420
  static inline bool killpg(pid_t pid, int signo) noexcept
    { return ignore_interruption<int, pid_t, int>(::killpg, pid, signo) != error_response; }
#endif

  using ::getopt;

  // fcntl.h
  using ::fcntl;



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
    return rval == nullptr ? uid_t(error_response) : rval->pw_uid;
  }

  static inline gid_t getgroupid(const char* name) noexcept
  {
    group* rval = posix::getgrnam(name);
    return rval == nullptr ? gid_t(error_response) : rval->gr_gid;
  }

  static inline bool useringroup(const char* groupname, const char* username)
  {
    if(username == nullptr)
      return false;
    group* grp = posix::getgrnam(groupname);
    if(grp == nullptr)
      return false;
    for(char** member = grp->gr_mem; *member != nullptr; ++member)
      if(!::strcmp(*member, username))
        return true;
    return false;
  }

// longcuts
  namespace Signal
  {
    enum EId : error_t
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

    enum ECode : error_t
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

#if (_XOPEN_SOURCE) >= 400
    static inline bool raise(EId id) noexcept
      { return ::raise(id) == success_response; }
#endif

    static inline bool send(pid_t pid, EId id, int value = 0) noexcept
#if (_XOPEN_SOURCE) >= 500 && defined(_XOPEN_SOURCE_EXTENDED)
      { return posix::sigqueue(pid, id, {value}); }
#else
      { (void)value; return posix::kill(pid, id); }
#endif

    static inline bool sendpg(pid_t pgid, EId id) noexcept
      { return posix::killpg(pgid, id); }
  }

// POSIX wrappers
  static inline bool dup2(fd_t a, fd_t b) noexcept
    { return ignore_interruption(::dup2, a, b) != error_response; }

  static inline bool close(fd_t fd) noexcept
    { return ignore_interruption(::close, fd) == success_response; }

  static inline ssize_t write(fd_t fd, const void* buffer, size_t length) noexcept
    { return ignore_interruption(::write, fd, buffer, length); }

  static inline ssize_t read(fd_t fd, void* buffer, size_t length) noexcept
    { return ignore_interruption(::read, fd, buffer, length); }

// POSIX wrappers
  static inline bool chmod(const char* path, mode_t mode) noexcept
    { return ignore_interruption(::chmod, path, mode) == success_response; }

  static inline bool chown(const char* path, uid_t owner, gid_t group) noexcept
    { return ignore_interruption(::chown, path, owner, group) == success_response; }

  static inline bool stat(const char* path, struct stat* status) noexcept
    { return ::stat(path, status) == success_response; }


// variadic POSIX wrappers
  template<typename... ArgTypes>
#if defined(SA_RESTART) && defined(DISABLE_INTERRUPTED_WRAPPER)
  constexpr fd_t open(const char *path, int oflag, ArgTypes... args) noexcept
    { return ::open(path, oflag, args...); }
#else
  static inline fd_t open(const char *path, int oflag, ArgTypes... args) noexcept
  {
    fd_t rval = error_response;
    do {
      rval = ::open(path, oflag, args...);
    } while(rval == error_response && errno == std::errc::interrupted);
    return rval;
  }
#endif

  template<typename IntType, typename... ArgTypes>
#if defined(SA_RESTART) && defined(DISABLE_INTERRUPTED_WRAPPER)
  constexpr int ioctl(fd_t fd, IntType request, ArgTypes... args) noexcept
  {
    static_assert(std::is_integral<IntType>::value, "Has to be an integer type!");
    return ::ioctl(fd, request, args...);
  }
#else
  static inline int ioctl(fd_t fd, IntType request, ArgTypes... args) noexcept
  {
    static_assert(std::is_integral<IntType>::value, "Has to be an integer type!");
    int rval = error_response;
    do {
      rval = ::ioctl(fd, request, args...);
    } while(rval == error_response && errno == std::errc::interrupted);
    return rval;
  }
#endif

  enum class seek
  {
    relative = SEEK_CUR,
    from_end = SEEK_END,
    absolute = SEEK_SET,
  };
  // POSIX STREAM wrappers
  static inline FILE* fopen(const char* filename, const char* mode) noexcept
    { return ignore_interruption<FILE, const char*, const char*>(::fopen, filename, mode); }

  static inline bool fclose(FILE* stream) noexcept
    { return ignore_interruption(::fclose, stream) != error_response; }

  static inline bool feof(FILE* stream) noexcept
    { return ::feof(stream); }

  static inline bool fseek(FILE* stream, off_t offset, posix::seek whence) noexcept
    { return ::fseeko(stream, offset, int(whence)) == success_response; }

  static inline bool fgets(char* s, int n, FILE* stream) noexcept
    { return ignore_interruption<char, char*, int, FILE*>(::fgets, s, n, stream) != nullptr; }

  static inline int fgetc(FILE* stream) noexcept
    { return ignore_interruption(::fgetc, stream); }

  static inline size_t fread(void* buffer, size_t buffer_size, size_t count, FILE* stream) noexcept
    { return ignore_interruption(::fread, buffer, buffer_size, count, stream); }

  static inline size_t fwrite(const void* buffer, size_t buffer_size, size_t count, FILE* stream) noexcept
    { return ignore_interruption(::fwrite, buffer, buffer_size, count, stream); }

  // sys/wait.h

#if !defined(WCONTINUED) /* if lacking XSI Conformance */
# define WCONTINUED 0
# define WIFCONTINUED(x) false
#endif

  static inline pid_t waitpid(pid_t pid, int* stat_loc, int options) noexcept
    { return ignore_interruption(::waitpid, pid, stat_loc, options); }


  // shortcut
  static inline bool donotblock(fd_t fd)
  {
#if defined(O_NONBLOCK) // POSIX
    int flags = posix::fcntl(fd, F_GETFL, 0);
    return flags != posix::error_response &&
           posix::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != posix::error_response;
#else // pre-POSIX
    int flags = 1;
    return ioctl(fd, FIOBIO, &flags) != posix::error_response);
#endif
  }
}

#endif // POSIX_HELPERS_H
