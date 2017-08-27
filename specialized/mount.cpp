#include "mount.h"

#include <sys/param.h>
#include	<sys/mount.h>

#include <cxxutils/posix_helpers.h>

#if defined(__linux__)

// POSIX++
#include <cstring>

// STL
#include <list>
#include <string>

//PDTK
#include <cxxutils/hashing.h>

int mount(const char* device,
          const char* path,
          const char* filesystem,
          const char* options) noexcept
{
  std::list<std::string> fslist;
  std::string optionlist;
  unsigned long mountflags = 0;

  for(const char* pos = filesystem, *next = nullptr; pos != nullptr; pos = next)
  {
    if(*pos == ',')
      ++pos;
    next = std::strchr(pos, ',');

    if(next == nullptr) // end of string
      fslist.emplace_back(pos);
    else // not end of string
      fslist.emplace_back(pos, posix::size_t(next - pos));
  }
  for(const char* pos = options, *next = nullptr; pos != nullptr; pos = next)
  {
    if(*pos == ',')
      ++pos;
    std::string option;
    next = std::strchr(pos, ',');
    if(next == nullptr) // end of string
      option.assign(pos);
    else // not end of string
      option.assign(pos, posix::size_t(next - pos));

    switch(hash(option))
    {
      case "defaults"_hash: break;
      case "ro"_hash        : mountflags |= MS_RDONLY     ; break;
      case "noatime"_hash   : mountflags |= MS_NOATIME    ; break;
      case "nodiratime"_hash: mountflags |= MS_NODIRATIME ; break;
      case "nodev"_hash     : mountflags |= MS_NODEV      ; break;
      case "nosuid"_hash    : mountflags |= MS_NOSUID     ; break;
      case "noexec"_hash    : mountflags |= MS_NOEXEC     ; break;
      default:
        optionlist.append(option).append(1, ',');
    }
  }

  if(optionlist.back() == ',') // find trailing ','
    optionlist.pop_back(); // remove trailing ','

  for(const std::string& fs : fslist)
    if(mount(device, path, fs.c_str(), mountflags, optionlist.c_str()) == posix::success_response)
      return posix::success_response;

  return posix::error_response;
}

#elif defined(NETBSD)
int mount(const char* device,
          const char* path,
          const char* filesystem,
          const char* options) noexcept
{
// int mount(int type, const char* dir, int flags, caddr_t data);
  return posix::error_response;
}

#elif defined(BSD) || defined(__APPLE__)
int mount(const char* device,
          const char* path,
          const char* filesystem,
          const char* options) noexcept
{
// int mount(const char* type, const char* dir, int flags, void* data);
  return posix::error_response;
}
#endif
