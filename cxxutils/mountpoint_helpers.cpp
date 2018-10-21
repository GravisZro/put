#include "mountpoint_helpers.h"

#include "posix_helpers.h"


const char* procfs_path = nullptr;
const char* sysfs_path  = nullptr;
const char* devfs_path  = nullptr;
const char* scfs_path   = nullptr;

int init_paths(void)
{
  procfs_path = "/proc";
  sysfs_path = "/sys";
  devfs_path = "/dev";
  scfs_path = "/svc";

  return posix::success_response;
}

static int path_init_ok = init_paths();
