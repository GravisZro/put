#include "mount.h"

// PUT
#include <specialized/osdetect.h>
#include <cxxutils/posix_helpers.h>

#if defined(__linux__)    /* Linux  */ || \
    defined(BSD)          /* *BSD   */ || \
    defined(__darwin__)   /* Darwin */
# include <sys/param.h>
# include <sys/mount.h>
#endif

// POSIX++
#include <cstring>

// STL
#include <list>
#include <string>

// PUT
#include <cxxutils/hashing.h>

#if defined(__linux__)
#define MNT_RDONLY        MS_RDONLY
#define MNT_NOEXEC        MS_NOEXEC
#define MNT_NOSUID        MS_NOSUID
#define MNT_NODEV         MS_NODEV
#define MNT_SYNCHRONOUS   MS_SYNCHRONOUS
#define MNT_NOATIME       MS_NOATIME
#define MNT_RELATIME      MS_RELATIME
#elif defined(BSD) && !defined(MNT_RDONLY)
#define MNT_RDONLY        M_RDONLY
#define MNT_NOEXEC        M_NOEXEC
#define MNT_NOSUID        M_NOSUID
#define MNT_NODEV         M_NODEV
#define MNT_SYNCHRONOUS   M_SYNCHRONOUS
#define MNT_RDONLY        M_RDONLY
#endif

bool mount(const char* device,
           const char* path,
           const char* filesystem,
           const char* options) noexcept
{
  unsigned long mountflags = 0;
  std::list<std::string> fslist;
#if defined(__linux__)
  std::string optionlist;
#endif

  for(const char* pos = filesystem, *next = NULL; pos != NULL; pos = next)
  {
    if(*pos == ',')
      ++pos;
    next = std::strchr(pos, ',');

    if(next == NULL) // end of string
      fslist.emplace_back(pos);
    else // not end of string
      fslist.emplace_back(pos, posix::size_t(next - pos));
  }

  for(const char* pos = options, *next = NULL; pos != NULL; pos = next)
  {
    if(*pos == ',')
      ++pos;
    std::string option;
    next = std::strchr(pos, ',');
    if(next == NULL) // end of string
      option.assign(pos);
    else // not end of string
      option.assign(pos, posix::size_t(next - pos));

    switch(hash(option))
    {
      case "defaults"_hash: break;

// Generic
#ifdef MNT_RDONLY
      case "ro"_hash          : mountflags |= MNT_RDONLY      ; break;
      case "rw"_hash          : mountflags &= 0^MNT_RDONLY    ; break;
#endif
#ifdef MNT_NOEXEC
      case "noexec"_hash      : mountflags |= MNT_NOEXEC      ; break;
#endif
#ifdef MNT_NOSUID
      case "nosuid"_hash      : mountflags |= MNT_NOSUID      ; break;
#endif
#ifdef MNT_NODEV
      case "nodev"_hash       : mountflags |= MNT_NODEV       ; break;
#endif
#ifdef MNT_SYNCHRONOUS
      case "sync"_hash        : mountflags |= MNT_SYNCHRONOUS ; break;
#endif

// Linux Only
#ifdef MS_NODIRATIME
      case "nodiratime"_hash  : mountflags |= MS_NODIRATIME   ; break;
#endif
#ifdef MS_MOVE
      case "move"_hash        : mountflags |= MS_MOVE         ; break;
#endif
#ifdef MS_BIND
      case "bind"_hash        : mountflags |= MS_BIND         ; break;
#endif
#ifdef MS_REMOUNT
      case "remount"_hash     : mountflags |= MS_REMOUNT      ; break;
#endif
#ifdef MS_MANDLOCK
      case "mandlock"_hash    : mountflags |= MS_MANDLOCK     ; break;
#endif
#ifdef MS_DIRSYNC
      case "dirsync"_hash     : mountflags |= MS_DIRSYNC      ; break;
#endif
#ifdef MS_REC
      case "rec"_hash         : mountflags |= MS_REC          ; break;
#endif
#ifdef MS_SILENT
      case "silent"_hash      : mountflags |= MS_SILENT       ; break;
#endif
#ifdef MS_POSIXACL
      case "posixacl"_hash    : mountflags |= MS_POSIXACL     ; break;
#endif
#ifdef MS_UNBINDABLE
      case "unbindable"_hash  : mountflags |= MS_UNBINDABLE   ; break;
#endif
#ifdef MS_PRIVATE
      case "private"_hash     : mountflags |= MS_PRIVATE      ; break;
#endif
#ifdef MS_SLAVE
      case "slave"_hash       : mountflags |= MS_SLAVE        ; break;
#endif
#ifdef MS_SHARED
      case "shared"_hash      : mountflags |= MS_SHARED       ; break;
#endif
#ifdef MS_KERNMOUNT
      case "kernmount"_hash   : mountflags |= MS_KERNMOUNT    ; break;
#endif
#ifdef MS_I_VERSION
      case "iversion"_hash    : mountflags |= MS_I_VERSION    ; break;
#endif
#ifdef MS_STRICTATIME
      case "strictatime"_hash : mountflags |= MS_STRICTATIME  ; break;
#endif
#ifdef MS_LAZYTIME
      case "lazytime"_hash    : mountflags |= MS_LAZYTIME     ; break;
#endif
#ifdef MS_ACTIVE
      case "active"_hash      : mountflags |= MS_ACTIVE       ; break;
#endif
#ifdef MS_NOUSER
      case "nouser"_hash      : mountflags |= MS_NOUSER       ; break;
#endif

// BSDs
#ifdef MNT_UNION
      case "union"_hash       : mountflags |= MNT_UNION       ; break;
#endif
#ifdef MNT_HIDDEN
      case "hidden"_hash      : mountflags |= MNT_HIDDEN      ; break;
#endif
#ifdef MNT_NOCOREDUMP
      case "nocoredump"_hash  : mountflags |= MNT_NOCOREDUMP  ; break;
#endif
#ifdef MNT_NOATIME
      case "noatime"_hash     : mountflags |= MNT_NOATIME     ; break;
#endif
#ifdef MNT_RELATIME
      case "relatime"_hash    : mountflags |= MNT_RELATIME    ; break;
      case "norelatime"_hash  : mountflags &= 0^MNT_RELATIME  ; break;
#endif
#ifdef MNT_NODEVMTIME
      case "nodevmtime"_hash  : mountflags |= MNT_NODEVMTIME  ; break;
#endif
#ifdef MNT_SYMPERM
      case "symperm"_hash     : mountflags |= MNT_SYMPERM     ; break;
#endif
#ifdef MNT_ASYNC
      case "async"_hash       : mountflags |= MNT_ASYNC       ; break;
#endif
#ifdef MNT_LOG
      case "log"_hash         : mountflags |= MNT_LOG         ; break;
#endif
#ifdef MNT_EXTATTR
      case "extattr"_hash     : mountflags |= MNT_EXTATTR     ; break;
#endif
#ifdef MNT_SNAPSHOT
      case "snapshot"_hash    : mountflags |= MNT_SNAPSHOT    ; break;
#endif
#ifdef MNT_SUIDDIR
      case "suiddir"_hash     : mountflags |= MNT_SUIDDIR     ; break;
#endif
#ifdef MNT_FORCE
      case "force"_hash       : mountflags |= MNT_FORCE       ; break;
#endif
#ifdef MNT_NOCLUSTERR
      case "noclusterr"_hash  : mountflags |= MNT_NOCLUSTERR  ; break;
#endif
#ifdef MNT_NOCLUSTERW
      case "noclusterw"_hash  : mountflags |= MNT_NOCLUSTERW  ; break;
#endif
#ifdef MNT_EXTATTR
      case "extattr"_hash     : mountflags |= MNT_EXTATTR     ; break;
#endif
#ifdef MNT_DETACH
      case "detach"_hash      : mountflags |= MNT_DETACH      ; break;
#endif
#ifdef MNT_SOFTDEP
      case "softdep"_hash     : mountflags |= MNT_SOFTDEP     ; break;
#endif
#ifdef MNT_WXALLOWED
      case "wxallowed"_hash   : mountflags |= MNT_WXALLOWED   ; break;
#endif
      default:
#if defined(__linux__)
        optionlist.append(option).append(1, ',');
#else
#endif
        break;
    }
  }

  //MNT_UPDATE, MNT_RELOAD, and MNT_GETARGS

#if defined(__linux__)
  if(optionlist.back() == ',') // find trailing ','
    optionlist.pop_back(); // remove trailing ','
#endif

  for(const std::string& fs : fslist)
  {
#if defined(__linux__)
    if(mount(device, path, fs.c_str(), mountflags, optionlist.c_str()) == posix::success_response)
      return true;
#else

#endif
  }

  return false;
}

bool unmount(const char* path) noexcept
{
#if defined(__linux__)
  return umount(path) != posix::success_response;
#else
  return unmount(path, 0);
#endif
}
