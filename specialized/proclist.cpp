#include "proclist.h"

// PDTK
#include <cxxutils/posix_helpers.h>

#if defined(__linux__) // Linux

#include <dirent.h>
#include <cstdlib>

int proclist(pid_t* list, int max_length)
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

#elif defined(BSD) || defined(__APPLE__) // *BSD or Apple

#elif defined(__unix__)

//system("ps -A -o pid=");

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif

