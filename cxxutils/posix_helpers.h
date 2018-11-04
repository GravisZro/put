#ifndef POSIX_HELPERS_H
#define POSIX_HELPERS_H

// POSIX
#include <sys/types.h> // system specific types
#include <sys/stat.h>  // stat stuff
#include <pwd.h>       // passwd stuff
#include <grp.h>       // group stuff
#include <unistd.h>
#include <fcntl.h>

// POSIX-esque?
#include <sys/ioctl.h>

// POSIX++
#include <cstring> // for useringroup()
#include <csignal>
#include <cstdio>  // file descriptor I/O
#include <cstdint> // need standard types

// PUT
#include "error_helpers.h"
#include "signal_helpers.h"

// <=== OS IDENTIFIERS ===>

#if !defined(__unix__) && defined(__unix) // generic unix
# define __unix__
#endif

// === Free open source ===

#if !defined(__linux__) && (defined(__linux) || defined (linux)) // Linux
#define __linux__

#elif !defined(__kfreebsd__) && defined(__FreeBSD_kernel__) && defined(__GLIBC__) // kFreeBSD
# define __kfreebsd__

#elif !defined(__minix__) && defined(__minix) // MINIX
# define __minix__

#elif !defined(__plan9__) && defined(EPLAN9) // Plan 9
# define __plan9__

#elif !defined(__ecos__) && defined(__ECOS) // eCos
# define __ecos__

#elif !defined(__syllable__) && defined(__SYLLABLE__) // Syllable
# define __syllable__

// #if defined(__FreeBSD__)   // FreeBSD
// #if defined(__NetBSD__)    // NetBSD
// #if defined(__OpenBSD__)   // OpenBSD
// #if defined(__DragonFly__) // DragonFly BSD

// === Commercial open source ===
#elif !defined(__darwin__) && defined(__APPLE__) && defined(__MACH__) // Darwin
# define __darwin__
# define __unix__

#elif defined(__sun) || defined(sun)
# if !defined(__solaris__) && defined(__SVR4) // Solaris
#  define __solaris__
# elif !defined(__sunos__) // SunOS
#  define __sunos__
# endif

#elif !defined(__sco_openserver__) && defined(_SCO_DS) // SCO OpenServer
# define __sco_openserver__

// #if defined(__bsdi__) // BSD/OS

// === Commercial closed source ===
#elif !defined(__zos__) && (defined(__MVS__) || defined(__HOS_MVS__) || defined(__TOS_MVS__)) // z/OS
# define __zos__

#elif !defined(__tru64__) && defined(__osf__) || defined(__osf) // Tru64 (OSF/1)
# define __tru64__

#elif !defined(__hpux__) && (defined(__hpux) || defined(hpux)) // HP-UX
# define __hpux__

#elif !defined(__irix__) && (defined(__sgi) || defined(sgi)) // IRIX
# define __irix__

#elif !defined(__unixware__) && (defined(_UNIXWARE7) || defined(sco)) // UnixWare
# define __unixware__

#elif !defined(__dynix__) && (defined(_SEQUENT_) || defined(sequent)) // DYNIX
# define __dynix__

#elif !defined(__mpeix__) && (defined(__mpexl) || defined(mpeix)) // MPE/iX
# define __mpeix__

#elif !defined(__vms__) && (defined(__VMS) || defined(VMS)) // VMS
# define __vms__

#elif !defined(__aix__) && defined(_AIX) // AIX
# define __aix__

#elif !defined(__dcosx__) && defined(pyr) // DC/OSx
# define __dcosx__

#elif !defined(__reliant_unix__) && defined(sinux) // Reliant UNIX
# define __reliant_unix__

#elif !defined(__interix__) && defined(__INTERIX) // Interix
# define __interix__

#elif !defined(__morphos__) && defined(__MORPHOS__) // MorphOS
# define __morphos__

// redundant
#elif !defined(__OS2__) && (defined(__TOS_OS2__) || defined(_OS2) || defined(OS2)) // OS/2
# define __OS2__

#elif !defined(__ultrix__) && (defined(__ultrix) || defined(ultrix)) // Ultrix
# define __ultrix__

#elif !defined(__dgux__) && (defined(__DGUX__) || defined(DGUX)) // DG/UX
# define __dgux__

#elif !defined(__amigaos__) && defined(AMIGA) // AmigaOS
# define __amigaos__

#elif !defined(__QNX__) && defined(__QNXNTO__) // QNX
# define __QNX__

// #if defined(__BEOS__)    // BeOS
// #if defined(__Lynx__)    // LynxOS
// #if defined(__nucleus__) // Nucleus RTOS
// #if defined(__VOS__)     // Stratus VOS

#endif
// </=== OS IDENTIFIERS ===>

static_assert(sizeof(::size_t) == sizeof(std::size_t), "STL's size_t doesn't match the C standard!");
static_assert(sizeof(::size_t) == sizeof(::off_t), "size_t not the same is as off_t!");
static_assert(sizeof(::size_t) == sizeof(::ssize_t), "size_t not the same is as ssize_t!");

namespace posix
{
  using ::size_t;
  using ::ssize_t;
  using ::off_t;

  // unistd.h
  using ::access;
  using ::faccessat;
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
#if (_XOPEN_SOURCE - 0) >= 700
  using ::fexecve;
#endif

  /*
  int execl(const char *path, const char *arg0, ... /, (char *)0 /);
//  int execle(const char *path, const char *arg0, ... /, (char *)0, char *const envp[]/);
  int execlp(const char *file, const char *arg0, ... /, (char *)0 /);

  // scripts
  static inline bool execvp(const char *file, char *const argv[]);

  // binaries
  static inline bool execv(const char *path, char *const argv[]);
  static inline bool execve(const char *path, char *const argv[], char *const envp[]);
  static inline bool fexecve(int fd, char *const argv[], char *const envp[]);
*/

  typedef int fd_t;
  static const fd_t invalid_descriptor = error_response;

  static inline bool fchdir(fd_t fd) noexcept
    { return ignore_interruption<int, fd_t>(::fchdir, fd) != error_response; }


  static inline bool setuid(uid_t uid) noexcept
    { return ignore_interruption<int, uid_t>(::setuid, uid) != error_response; }

  static inline bool seteuid(uid_t uid) noexcept
    { return ignore_interruption<int, uid_t>(::seteuid, uid) != error_response; }

  static inline bool setgid(gid_t gid) noexcept
    { return ignore_interruption<int, gid_t>(::setgid, gid) != error_response; }

  static inline bool setegid(gid_t gid) noexcept
    { return ignore_interruption<int, gid_t>(::setegid, gid) != error_response; }

  using ::getuid;
  using ::geteuid;
  using ::getgid;
  using ::getegid;

  static inline bool pipe(fd_t fildes[2]) noexcept
    { return ignore_interruption<int, int*>(::pipe, fildes) != error_response; }

  using ::dup;
  //using ::dup2; EINTR

  using ::fork;
#if defined(_XOPEN_SOURCE_EXTENDED)
  static inline bool sigqueue(pid_t pid, int signo, union sigval value) noexcept
    { return ignore_interruption<int, pid_t, int, union sigval>(::sigqueue, pid, signo, value) != error_response; }
#endif
  static inline bool kill(pid_t pid, int signo) noexcept
    { return ignore_interruption<int, pid_t, int>(::kill, pid, signo) != error_response; }

  static inline bool killpg(pid_t pid, int signo) noexcept
    { return ignore_interruption<int, pid_t, int>(::killpg, pid, signo) != error_response; }

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
      if(!std::strcmp(*member, username))
        return true;
    return false;
  }

// longcuts
  namespace signal
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

    static inline bool raise(EId id) noexcept
      { return std::raise(id) == success_response; }

    static inline bool send(pid_t pid, EId id, int value = 0) noexcept
#if defined(_XOPEN_SOURCE_EXTENDED)
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


// variadic POSIX wrappers
  template<typename... ArgTypes>
#if defined(SA_RESTART) && !defined(INTERRUPTED_WRAPPER)
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
#if defined(SA_RESTART) && !defined(INTERRUPTED_WRAPPER)
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

  // POSIX wrappers
  static inline std::FILE* fopen(const char* filename, const char* mode) noexcept
    { return ignore_interruption<std::FILE, const char*, const char*>(std::fopen, filename, mode); }

  static inline bool fclose(std::FILE* stream) noexcept
    { return ignore_interruption(std::fclose, stream) != error_response; }

  static inline bool fgets(char* s, int n, std::FILE* stream) noexcept
    { return ignore_interruption<char, char*, int, std::FILE*>(std::fgets, s, n, stream) != nullptr; }

  static inline int fgetc(FILE* stream) noexcept
    { return ignore_interruption(std::fgetc, stream); }

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
