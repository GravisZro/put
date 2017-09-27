#include "module.h"

#include <cxxutils/posix_helpers.h>

#if defined(__linux__)

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

int init_module(void *module_image, unsigned long len,
                const char *param_values);

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
      if(mem != nullptr)
        rval = init_module(mem, state.st_size, module_arguments);
      ::munmap(mem, state.st_size);
    }
    posix::close(fd);
  }
  return rval;
}

#elif defined(__hpux) // HP-UX
#error No kernel module operations code exists in PDTK for HP-UX!  Please submit a patch!

#elif defined(_AIX) // IBM AIX
// https://www.ibm.com/developerworks/aix/library/au-kernelext.html
#error No kernel module operations code exists in PDTK for IBM AIX!  Please submit a patch!

#elif defined(__APPLE__) // Darwin
// kextload and kextunload
#error No kernel module operations code exists in PDTK for Darwin!  Please submit a patch!

#elif defined(__sun) && defined(__SVR4) // Solaris / OpenSolaris / OpenIndiana / illumos
#error No kernel module operations code exists in PDTK for Solaris!  Please submit a patch!

#elif defined(__FreeBSD__) || defined(__DragonFly__) // FreeBSD and DragonFly BSD
// https://man.openbsd.org/FreeBSD-11.0/kldload.2
// https://man.openbsd.org/FreeBSD-11.0/kldunload.2
// kldload and kldunload
#error No kernel module operations code exists in PDTK for FreeBSD/DragonFly BSD!  Please submit a patch!

#elif defined(__OpenBSD__)
// https://man.openbsd.org/OpenBSD-5.4/lkm.4
// ioctl on /dev/lkm
#error No kernel module operations code exists in PDTK for OpenBSD!  Please submit a patch!

#elif defined(__NetBSD__) // NetBSD
// https://man.openbsd.org/NetBSD-7.1/module.9
// module_load and module_unload
#error No kernel module operations code exists in PDTK for NetBSD!  Please submit a patch!

#elif defined(BSD)
#error Unrecognized BSD derivative!

#elif defined(__unix__)
#error Unrecognized UNIX variant!

#else
#error This platform is not supported.
#endif
