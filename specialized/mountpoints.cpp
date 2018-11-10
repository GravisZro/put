#include "mountpoints.h"

// POSIX++
#include <climits>
#include <cstring>

// PUT
#include <cxxutils/hashing.h>
#include <specialized/fstable.h>
#include <specialized/osdetect.h>

char* procfs_path = nullptr;
char* sysfs_path  = nullptr;
char* devfs_path  = nullptr;
char* scfs_path   = nullptr;

inline void assign_data(char*& target, const char* str) noexcept
{
  if(target != nullptr)
    ::free(target);
  size_t length = ::strnlen(str, PATH_MAX) + 1;
  target = static_cast<char*>(::malloc(length));
  std::strncpy(target, str, length);
}

#if defined(__linux__)    /* Linux    */ || \
    defined(__minix__)    /* MINIX    */ || \
    defined(__solaris__)  /* Solaris  */ || \
    defined(__hpux__)     /* HP-UX    */ || \
    defined(__irix__)     /* IRIX     */

#ifndef MOUNT_TABLE_FILE
# if defined(__solaris__) /* Solaris  */ || \
     defined(__hpux__)    /* HP-UX    */
# define MOUNT_TABLE_FILE   "/etc/mnttab"
#else
# define MOUNT_TABLE_FILE   "/etc/mtab"
#endif

bool initialize_paths(void) noexcept
{
  std::list<struct fsentry_t> table;
  if(!parse_table(table, MOUNT_TABLE_FILE))
    return false;

  for(const fsentry_t& entry : table)
  {
    switch(hash(entry.filesystems))
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
#elif defined(BSD4_4)       /* *BSD     */ || \
      defined(__darwin__)   /* Darwin   */ || \
      defined(__tru64__)    /* Tru64    */

#include <sys/types.h>

#if defined(__NetBSD__) && KERNEL_VERSION_CODE < KERNEL_VERSION(3,0,0)
#include <sys/statvfs.h>
#define statfs statvfs;
#define getfsstat(a,b,c) getvfsstat(a,b,c)
#else
#include <sys/mount.h>
#endif

bool initialize_paths(void) noexcept
{
  int count = getfsstat(NULL, 0, 0);
  if(count < 0)
    return posix::error_response;
  struct statfs* buffer = static_cast<struct statfs*>(::malloc(sizeof(struct statfs) * count));
  if(buffer == nullptr)
    return false;

  for(int i = 0; i < count; ++i)
  {
    switch(hash(buffer[i].f_mntfromname))
    {
      case "proc"_hash:
      case "procfs"_hash:
        assign_data(procfs_path, buffer[i].f_mntonname);
        break;
      case "devtmpfs"_hash:
      case "devfs"_hash:
        assign_data(devfs_path, buffer[i].f_mntonname);
        break;
      case "sysfs"_hash:
        assign_data(sysfs_path, buffer[i].f_mntonname);
        break;
      case "scfs"_hash:
        assign_data(scfs_path, buffer[i].f_mntonname);
        break;
    }
  }
  return true;
}

#else

#endif

static bool onstart = initialize_paths();
