#include "mountpoints.h"

// PUT
#include <put/cxxutils/hashing.h>
#include <put/specialized/fstable.h>

char* procfs_path = nullptr;
char* sysfs_path  = nullptr;
char* devfs_path  = nullptr;
char* scfs_path   = nullptr;

inline void assign_data(char*& target, const char* str) noexcept
{
  if(target != nullptr)
    posix::free(target);
  size_t length = posix::strnlen(str, PATH_MAX) + 1;
  target = static_cast<char*>(posix::malloc(length));
  posix::strncpy(target, str, length);
}

bool reinitialize_paths(void) noexcept
{
  std::list<fsentry_t> table;
  if(!mount_table(table))
    return false;

  for(fsentry_t& entry : table)
  {
    switch(hash(entry.device))
    {
      case "proc"_hash:
      case "procfs"_hash:
        assign_data(procfs_path, entry.path);
        break;
      case "devtmpfs"_hash:
      case "devfs"_hash:
        assign_data(devfs_path, entry.path);
        break;
      case "sysfs"_hash:
        assign_data(sysfs_path, entry.path);
        break;
      case "scfs"_hash:
        assign_data(scfs_path, entry.path);
        break;
    }
  }
  return true;
}

static bool onstart = reinitialize_paths();
