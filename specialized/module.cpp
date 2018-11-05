#include "module.h"

// PUT
#include <specialized/osdetect.h>
#include <cxxutils/posix_helpers.h>

#if defined(__linux__)

#include <sys/mman.h>
#include <sys/syscall.h>

int load_module(const char* filename, const char* module_arguments)
{
  int rval = posix::error_response;
  posix::fd_t fd = posix::open(filename, O_RDONLY | O_CLOEXEC);
  if(fd != posix::error_response)
  {
    struct stat state;
    rval = ::fstat(fd, &state);
    if(rval == posix::success_response)
    {
      void* mem = ::mmap(0, state.st_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
      if(mem != NULL)
        rval = ::syscall(SYS_init_module, mem, state.st_size, module_arguments);
      ::munmap(mem, state.st_size);
    }
    posix::close(fd);
  }
  return rval;
}

#elif defined(__aix__) // IBM AIX
// https://www.ibm.com/developerworks/aix/library/au-kernelext.html
#error No kernel module operations code exists in PUT for IBM AIX!  Please submit a patch!

#elif defined(__APPLE__) // Darwin
// kextload and kextunload
#error No kernel module operations code exists in PUT for Darwin!  Please submit a patch!

#elif defined(__solaris__) // Solaris / OpenSolaris / OpenIndiana / illumos
#error No kernel module operations code exists in PUT for Solaris!  Please submit a patch!

#elif defined(__FreeBSD__) || defined(__DragonFly__) // FreeBSD and DragonFly BSD
// https://man.openbsd.org/FreeBSD-11.0/kldload.2
// https://man.openbsd.org/FreeBSD-11.0/kldunload.2
// kldload and kldunload
#error No kernel module operations code exists in PUT for FreeBSD/DragonFly BSD!  Please submit a patch!

#elif defined(__OpenBSD__)
// https://man.openbsd.org/OpenBSD-5.4/lkm.4
// ioctl on /dev/lkm
#error No kernel module operations code exists in PUT for OpenBSD!  Please submit a patch!

#elif defined(__NetBSD__) // NetBSD
// https://man.openbsd.org/NetBSD-7.1/module.9
// module_load and module_unload
#error No kernel module operations code exists in PUT for NetBSD!  Please submit a patch!

#elif defined(__unix__)
# error No kernel module operations code exists in PUT for this UNIX!  Please submit a patch!

#else
#error This platform is not supported.
#endif
