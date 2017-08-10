#include "proclist.h"

// PDTK
#include <cxxutils/posix_helpers.h>

#if defined(__linux__) // Linux

#include <dirent.h>
#include <cstdlib>

int proclist(pid_t* list, size_t max_length)
{
  DIR* dirp = ::opendir("/proc");
  if(dirp == nullptr)
    return posix::error_response;

  struct dirent* entry;
  int count = 0;

  while((entry = ::readdir(dirp)) != nullptr &&
        count < max_length)
  {
    if(entry->d_type == DT_DIR)
    {
      list[count] = std::atoi(entry->d_name);
      if(list[count])
        ++count;
    }
  }

  if(errno != posix::success_response)
    return posix::error_response;

  if(::closedir(dirp) == posix::error_response)
    return posix::error_response;
  return count;
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

int proclist(pid_t* list, size_t max_length)
{
  size_t length = 0;
  std::vector<struct kinfo_proc> proc_list;
  int request[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

  std::memset(list, 0, max_length);

  if(::sysctl(request, arraylength(request), nullptr, &length, nullptr, 0) != posix::success_response)
    return posix::error_response;

  if(length > max_length)
    return posix::error(std::errc::not_enough_memory);

  proc_list.resize(length);

  if(::sysctl(request, arraylength(request), proc_list.data(), &length, nullptr, 0) != posix::success_response)
    return posix::error_response;

  if(proc_list.size() >= length)
    return posix::error(std::errc::not_enough_memory);

  proc_list.resize(length);

  struct kinfo_proc* proc_pos = proc_list.data();
  for(pid_t* pos = list; pos != list + length; ++pos, ++proc_pos)
#if defined(__APPLE__)
    *pos = proc_pos->kp_proc.p_pid;
#else
    *pos = proc_pos->ki_pid;
#endif

  return posix::success_response;
}

#elif defined(__unix__)

//system("ps -A -o pid=");

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif

