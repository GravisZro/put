#include "proclist.h"

// PUT
#include <cxxutils/posix_helpers.h>
#include <specialized/osdetect.h>

#if defined(__FreeBSD__)  /* FreeBSD  */ || \
    defined(__OpenBSD__)  /* OpenBSD  */ || \
    defined(__NetBSD__)   /* NetBSD   */ || \
    defined(__darwin__)   /* Darwin   */

// Darwin structure documentation
// kinfo_proc : https://opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/sys/sysctl.h.auto.html

// BSD structure documentation
// kinfo_proc : http://fxr.watson.org/fxr/source/sys/user.h#L119

// *BSD/Darwin
#include <sys/sysctl.h>

// STL
#include <vector>

// PUT
#include <cxxutils/misc_helpers.h>

#if (defined(__FreeBSD__) && KERNEL_VERSION_CODE < KERNEL_VERSION(8,0,0)) || \
    (defined(__NetBSD__)  && KERNEL_VERSION_CODE < KERNEL_VERSION(1,5,0)) || \
    (defined(__OpenBSD__) && KERNEL_VERSION_CODE < KERNEL_VERSION(3,7,0))
typedef struct proc kinfo_proc;
#endif


bool proclist(std::set<pid_t>& list) noexcept
{
  size_t length = 0;
  std::vector<kinfo_proc> proc_list;
  int request[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

  list.clear();

  if(::sysctl(request, arraylength(request), NULL, &length, NULL, 0) != posix::success_response)
    return false;

  length /= sizeof(kinfo_proc); // length is returned in bytes
  proc_list.resize(length);
  if(proc_list.size() < length) // if resize failed to allocate the required memory
  {
    errno = int(std::errc::not_enough_memory);
    return false;
  }

  length *= sizeof(kinfo_proc); // length is now bytes
  if(::sysctl(request, arraylength(request), proc_list.data(), &length, NULL, 0) != posix::success_response || !length)
    return false;

  for(const kinfo_proc& proc_info : proc_list)
  {
#if defined(__darwin__)
    if(proc_info.kp_proc.p_pid > 0)
      list.emplace(proc_info.kp_proc.p_pid);
#else
    if(proc_info.ki_pid > 0)
      list.emplace(proc_info.ki_pid);
#endif
  }

  return true;
}

#elif defined(__linux__)    /* Linux    */ || \
      defined(__solaris__)  /* Solaris  */ || \
      defined(__aix__)      /* AIX      */ || \
      defined(__tru64__)    /* Tru64    */ || \
      defined(__hpux__)     /* HP-UX    */

// POSIX
#include <dirent.h>

// PUT
#include <specialized/mountpoints.h>

static_assert(sizeof(pid_t) <= sizeof(int), "insufficient storage type for maximum number of pids");

bool proclist(std::set<pid_t>& list) noexcept
{
  errno = posix::success_response; // clear errno since it's required in this function
  list.clear();

  if(procfs_path == nullptr) // safety check
    return false;

  DIR* dirp = ::opendir(procfs_path);
  if(dirp == NULL)
    return false;

  size_t length = 0;
  struct dirent* entry;
  while((entry = ::readdir(dirp)) != NULL)
    if(entry->d_type == DT_DIR && std::atoi(entry->d_name))
      ++length;

  if(posix::is_success())
  {
    ::rewinddir(dirp);

    while((entry = ::readdir(dirp)) != NULL)
      if(entry->d_type == DT_DIR && std::atoi(entry->d_name))
        list.emplace(std::atoi(entry->d_name));
  }

  if(::closedir(dirp) == posix::error_response)
    return false;

  return posix::is_success();
}

#elif defined(__unix__) /* Generic UNIX */


#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif

