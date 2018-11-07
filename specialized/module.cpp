#include "module.h"

// PUT
#include <specialized/osdetect.h>
#include <cxxutils/posix_helpers.h>

#if defined(__linux__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,2,0) // Linux 2.2+

#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/module.h>

inline int init_module26(void* module_image, unsigned long len, const char* param_values) noexcept
  { return int(::syscall(SYS_init_module, module_image, len, param_values)); }

inline int delete_module26(const char* name, int flags) noexcept
  { return int(::syscall(SYS_delete_module, name, flags)); }

inline int init_module22(const char* name, struct module *image) noexcept
  { return int(::syscall(SYS_init_module, name, image)); }

inline int delete_module22(const char* name) noexcept
  { return int(::syscall(SYS_delete_module, name)); }

inline int query_module(const char* name, int which, void* buf, size_t bufsize, size_t* ret) noexcept
  { return int(::syscall(SYS_query_module, name, which, buf, bufsize, ret)); }

inline caddr_t create_module(const char *name, size_t size) noexcept
  { return caddr_t(::syscall(SYS_create_module, name, size)); }

inline bool is_kernel26(void) noexcept
{
  posix::error_t oldval = errno;
  query_module(NULL, 0, NULL, 0, NULL);
  bool rval = errno == ENOSYS;
  errno = oldval;
  return rval;
}

int load_module(const std::string& filename, const std::string& module_arguments) noexcept
{
  int rval = posix::error_response;
  posix::fd_t fd = posix::open(filename.c_str(), O_RDONLY | O_CLOEXEC);
  if(fd != posix::error_response)
  {
    struct stat state;
    rval = ::fstat(fd, &state);
    if(rval == posix::success_response)
    {
      void* mem = ::mmap(0, state.st_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
      if(mem != NULL)
      {
        if(is_kernel26())
          rval = init_module26(mem, state.st_size, module_arguments.c_str());
        else
        {
          const char* name;
          module* image;

          rval = init_module22(name, image);
        }
        ::munmap(mem, state.st_size);
      }
    }
    posix::close(fd);
  }
  return rval;
}

int unload_module(const std::string& name) noexcept
{
  if(is_kernel26())
    return delete_module26(name.c_str(), O_NONBLOCK);
  else
    return delete_module22(name.c_str());
}

#elif defined(__aix__) // IBM AIX
// https://www.ibm.com/developerworks/aix/library/au-kernelext.html
#error No kernel module operations code exists in PUT for IBM AIX!  Please submit a patch!

#elif defined(__darwin__) // Darwin
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

#elif defined(__NetBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(5,0,0) // NetBSD 5+

// See also: http://netbsd.gw.com/cgi-bin/man-cgi?modctl++NetBSD-6.0
# include <sys/module.h>

int load_module(const std::string& filename, const std::string& module_arguments) noexcept
{
  modctl_load_t mod;
  mod.ml_filename = filename.c_str();

  if(module_arguments.empty())
  {
    mod.ml_flags = MODCTL_NO_PROP;
    mod.ml_props = nullptr;
    mod.ml_propslen = 0;
  }
  else
  {
    mod.ml_flags = 0;
    mod.ml_props = module_arguments.c_str();
    mod.ml_propslen = module_arguments.size();
  }

  return ::modctl(MODCTL_LOAD, &mod);
}

int unload_module(const std::string& name) noexcept
{
  return ::modctl(MODCTL_UNLOAD, name.c_str());
}

#else
#pragma message("Loadable Kernel Modules are not supported on this platform.")

int load_module(const std::string&, const std::string&) noexcept
{ return posix::error(ENOSYS); }

int unload_module(const std::string&) noexcept;
{ return posix::error(ENOSYS); }
#endif
