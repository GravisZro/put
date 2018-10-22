#include "proclist.h"

#if defined(__linux__) // Linux

// POSIX++
#include <cstdlib>

// POSIX
#include <dirent.h>

// PDTK
#include <cxxutils/mountpoint_helpers.h>

static_assert(sizeof(pid_t) <= sizeof(int), "insufficient storage type for maximum number of pids");

posix::error_t proclist(std::vector<pid_t>& list) noexcept
{
  list.clear();

  if(procfs_path == nullptr &&
    initialize_paths() == posix::error_response)
    return posix::error_response;


  DIR* dirp = ::opendir(procfs_path);
  if(dirp == NULL)
    return posix::error_response;

  size_t length = 0;
  struct dirent* entry;
  while((entry = ::readdir(dirp)) != NULL)
    if(entry->d_type == DT_DIR && std::atoi(entry->d_name))
      ++length;

  if(errno == posix::success_response)
  {
    list.reserve(length);
    if(list.capacity() < length) // if reserve failed to allocate the required memory
      return posix::error(std::errc::not_enough_memory);

    ::rewinddir(dirp);

    while((entry = ::readdir(dirp)) != NULL)
      if(entry->d_type == DT_DIR && std::atoi(entry->d_name))
        list.push_back(std::atoi(entry->d_name));
  }

  if(errno != posix::success_response)
  {
    error_t errback = errno;
    ::closedir(dirp);
    return posix::error(errback);
  }

  if(::closedir(dirp) == posix::error_response)
    return posix::error_response;

  return posix::success_response;
}

#elif defined(BSD) || defined(__APPLE__) // *BSD/Darwin

// Darwin structure documentation
// kinfo_proc : https://opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/sys/sysctl.h.auto.html

// BSD structure documentation
// kinfo_proc : http://fxr.watson.org/fxr/source/sys/user.h#L119

 // *BSD/Darwin
#include <sys/sysctl.h>

// STL
#include <vector>

// PDTK
#include <cxxutils/misc_helpers.h>

posix::error_t proclist(std::vector<pid_t>& list) noexcept
{
  size_t length = 0;
  std::vector<struct kinfo_proc> proc_list;
  int request[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

  list.clear();

  if(::sysctl(request, arraylength(request), NULL, &length, NULL, 0) != posix::success_response)
    return posix::error_response;

  proc_list.resize(length);
  if(proc_list.size() < length) // if resize failed to allocate the required memory
    return posix::error(std::errc::not_enough_memory);

  list.reserve(length);
  if(list.capacity() < length) // if reserve failed to allocate the required memory
    return posix::error(std::errc::not_enough_memory);

  if(::sysctl(request, arraylength(request), proc_list.data(), &length, NULL, 0) != posix::success_response)
    return posix::error_response;

  for(const kinfo_proc& proc_info : proc_list)
#if defined(__APPLE__)
    list.push_back(proc_info.kp_proc.p_pid);
#else
    list.push_back(proc_info.ki_pid);
#endif

  return posix::success_response;
}

#elif defined(__unix__)

//system("ps -A -o pid=");

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif

