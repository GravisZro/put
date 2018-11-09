#include "module.h"

// PUT
#include <specialized/osdetect.h>
#include <cxxutils/posix_helpers.h>

#if defined(__linux__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,2,0) // Linux 2.2+

// See also: https://www.systutorials.com/docs/linux/man/2-create_module/
// See also: https://www.systutorials.com/docs/linux/man/2-query_module/

// Linux
# include <linux/module.h>
# include <sys/mman.h>
# include <sys/syscall.h>
# include <sys/stat.h>

# if KERNEL_VERSION_CODE >= KERNEL_VERSION(2,6,0)
struct module;
struct exception_table_entry;

struct module_ref
{
  module* dep;     /* "parent" pointer */
  module* ref;     /* "child" pointer */
  module_ref* next_ref;
};

struct module_symbol
{
  unsigned long value;
  const char *name;
};

struct module
{
    unsigned long  size_of_struct;
    module*        next;
    const char*    name;
    unsigned long  size;
    long           usecount;
    unsigned long  flags;
    unsigned int   nsyms;
    unsigned int   ndeps;
    module_symbol* syms;
    module_ref*    deps;
    module_ref*    refs;
    int          (*init)(void);
    void         (*cleanup)(void);
    const exception_table_entry* ex_table_start;
    const exception_table_entry* ex_table_end;
#  ifdef __alpha__
    unsigned long gp;
#  endif
};
# endif

inline int init_module26(void* module_image, unsigned long len, const char* param_values) noexcept
  { return int(::syscall(SYS_init_module, module_image, len, param_values)); }

inline int delete_module26(const char* name, int flags) noexcept
  { return int(::syscall(SYS_delete_module, name, flags)); }

inline int init_module22(const char* name, module *image) noexcept
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

bool load_module(const std::string& filename, const std::string& module_arguments) noexcept
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
        else if (false) // 2.2+ loader code is far from complete
        {
          const char* name;
          module* image;

          rval = init_module22(name, image);
        }
        else
          rval = posix::error(EOPNOTSUPP); // not implemented!
        ::munmap(mem, state.st_size);
      }
    }
    posix::close(fd);
  }
  return rval == posix::success_response;
}

bool unload_module(const std::string& name) noexcept
{
  if(is_kernel26())
    return delete_module26(name.c_str(), O_NONBLOCK) == posix::success_response;
  else
    return delete_module22(name.c_str()) == posix::success_response;
}

#elif defined(__aix__) /* AIX */
// https://www.ibm.com/developerworks/aix/library/au-kernelext.html
# error No kernel module operations code exists in PUT for AIX!  Please submit a patch!

#elif defined(__solaris__) /* Solaris */
# error No kernel module operations code exists in PUT for Solaris!  Please submit a patch!

#elif defined(__DragonFly__) /* DragonFly BSD */ || \
     (defined(__FreeBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(3,0,0)) /* FreeBSD 3+ */
// POSIX++
# include <cstring>

// FreeBSD / DragonFly BSD
# include <sys/linker.h>
# include <kenv.h>

template<class T> constexpr const T& max(const T& a, const T& b) { return (a < b) ? b : a; }

bool load_module(const std::string& filename, const std::string& module_arguments) noexcept
{
  int fileid = kldload(filename.c_str());

  if(fileid == posix::error_response &&
     !module_arguments.empty())
  {
    char key[KENV_MNAMELEN], value[KENV_MVALLEN];
    const char* prev = module_arguments.c_str();
    const char* pos = std::strtok(prev, "= ");
    while(pos != NULL && *pos == '=') // if NOT at end AND found '=' instead of ' '
    {
      std::memset(key  , 0, sizeof(key  )); // clear key
      std::memset(value, 0, sizeof(value)); // clear value
      std::memcpy(key, pos, min(posix::size_t(pos - prev), KENV_MNAMELEN)); // copy key
      prev = pos;
      pos = std::strtok(NULL, " "); // find next ' '
      if(pos == NULL) // found end of string instead
        std::memcpy(value, prev, min(std::strlen(prev), KENV_MVALLEN)); // copy value
      else if(*pos == ' ') // found ' '
        std::memcpy(value, prev, min(posix::size_t(pos - prev), KENV_MVALLEN)); // copy value
      kenv(KENV_SET, key, value, std::strlen(value) + 1); // set key/value pair
    }
    if(pos == NULL || *pos != ' ')
      return true;
    else
      errno = int(std::errc::invalid_argument);
  }

  return false;
}

bool unload_module(const std::string& name) noexcept
{
  int fileid = kldfind(name.c_str());
  if(fileid == posix::error_response)
    return false;
  return kldunload(fileid) == posix::success_response;
}

#elif defined(__NetBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(5,0,0) // NetBSD 5+

// See also: http://netbsd.gw.com/cgi-bin/man-cgi?modctl++NetBSD-6.0

// NetBSD
# include <sys/module.h>

bool load_module(const std::string& filename, const std::string& module_arguments) noexcept
{
  modctl_load_t mod;
  mod.ml_filename = filename.c_str();

  if(module_arguments.empty())
  {
    mod.ml_flags = MODCTL_NO_PROP;
    mod.ml_props = NULL;
    mod.ml_propslen = 0;
  }
  else
  {
    mod.ml_flags = 0;
    mod.ml_props = module_arguments.c_str();
    mod.ml_propslen = module_arguments.size();
  }

  return ::modctl(MODCTL_LOAD, &mod) == posix::success_response;
}

bool unload_module(const std::string& name) noexcept
{
  return ::modctl(MODCTL_UNLOAD, name.c_str()) == posix::success_response;
}
#else

# if defined(__darwin__)   /* Darwin         */ || \
     defined(__OpenBSD__)  /* OpenBSD        */ || \
     defined(__FreeBSD__)  /* legacy FreeBSD */ || \
     defined(__NetBSD__)   /* legacy NetBSD  */
// Darwin: kmodload and kmodunload (AKA kextload and kextunload)
// OpenBSD: modload and modunload

// https://man.openbsd.org/OpenBSD-5.4/lkm.4
// https://github.com/ekouekam/openbsd-src/tree/master/sbin/modload
// OpenBSD ioctl on /dev/lkm

#  pragma message("No kernel module ELF loader implemented!  Please submit a patch!")
#  pragma message("Loadable Kernel Module support is not implemented on this platform.")
# else
#  pragma message("Loadable Kernel Modules are not supported on this platform.")
# endif

bool load_module(const std::string&, const std::string&) noexcept
{
  errno = EOPNOTSUPP;
  return false;
}

bool unload_module(const std::string&) noexcept
{
  errno = EOPNOTSUPP;
  return false;
}
#endif
