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


// Linux Only
#ifndef MS_NODIRATIME
# define MS_NODIRATIME 0
#endif
#ifndef MS_MOVE
# define MS_MOVE 0
#endif
#ifndef MS_BIND
# define MS_BIND 0
#endif
#ifndef MS_REMOUNT
# define MS_REMOUNT 0
#endif
#ifndef MS_MANDLOCK
# define MS_MANDLOCK 0
#endif
#ifndef MS_DIRSYNC
# define MS_DIRSYNC 0
#endif
#ifndef MS_REC
# define MS_REC 0
#endif
#ifndef MS_SILENT
# define MS_SILENT 0
#endif
#ifndef MS_POSIXACL
# define MS_POSIXACL 0
#endif
#ifndef MS_UNBINDABLE
# define MS_UNBINDABLE 0
#endif
#ifndef MS_PRIVATE
# define MS_PRIVATE 0
#endif
#ifndef MS_SLAVE
# define MS_SLAVE 0
#endif
#ifndef MS_SHARED
# define MS_SHARED 0
#endif
#ifndef MS_KERNMOUNT
# define MS_KERNMOUNT 0
#endif
#ifndef MS_I_VERSION
# define MS_I_VERSION 0
#endif
#ifndef MS_STRICTATIME
# define MS_STRICTATIME 0
#endif
#ifndef MS_LAZYTIME
# define MS_LAZYTIME 0
#endif
#ifndef MS_ACTIVE
# define MS_ACTIVE 0
#endif
#ifndef MS_NOUSER
# define MS_NOUSER 0
#endif

#ifndef MNT_UNION
# define MNT_UNION 0
#endif
#ifndef MNT_HIDDEN
# define MNT_HIDDEN 0
#endif
#ifndef MNT_NOCOREDUMP
# define MNT_NOCOREDUMP 0
#endif
#ifndef MNT_NOATIME
# define MNT_NOATIME 0
#endif
#ifndef MNT_RELATIME
# define MNT_RELATIME 0
#endif
#ifndef MNT_NODEVMTIME
# define MNT_NODEVMTIME 0
#endif
#ifndef MNT_SYMPERM
# define MNT_SYMPERM 0
#endif
#ifndef MNT_ASYNC
# define MNT_ASYNC 0
#endif
#ifndef MNT_LOG
# define MNT_LOG 0
#endif
#ifndef MNT_EXTATTR
# define MNT_EXTATTR 0
#endif
#ifndef MNT_SNAPSHOT
# define MNT_SNAPSHOT 0
#endif
#ifndef MNT_SUIDDIR
# define MNT_SUIDDIR 0
#endif
#ifndef MNT_NOCLUSTERR
# define MNT_NOCLUSTERR 0
#endif
#ifndef MNT_NOCLUSTERW
# define MNT_NOCLUSTERW 0
#endif
#ifndef MNT_EXTATTR
# define MNT_EXTATTR 0
#endif
#ifndef MNT_SOFTDEP
# define MNT_SOFTDEP 0
#endif
#ifndef MNT_WXALLOWED
# define MNT_WXALLOWED 0
#endif

#ifndef MNT_FORCE
# define MNT_FORCE 0
#endif
#ifndef MNT_DETACH
# define MNT_DETACH 0
#endif
#ifndef MNT_DETACH
# define MNT_EXPIRE 0
#endif
#ifndef UMOUNT_NOFOLLOW
# define UMOUNT_NOFOLLOW 0
#endif

constexpr posix::size_t section_length(const char* start, const char* end)
  { return end == NULL ? std::strlen(start) : posix::size_t(end - start); }

bool mount(const char* device,
           const char* path,
           const char* filesystem,
           const char* options) noexcept
{
  std::list<std::string> fslist;
  char *pos, *next;
#if defined(__linux__)
  std::string optionlist;
  unsigned long mountflags = 0;
#else
  int mountflags = 0;
#endif

  pos = const_cast<char*>(filesystem);
  next = std::strtok(pos, ",");
  do
  {
    fslist.emplace_back(pos, section_length(pos, next));
    pos = next;
    next = std::strtok(NULL, ",");
  } while(pos != NULL && ++pos);

  if(options != nullptr)
  {
    pos = const_cast<char*>(options);
    next = std::strtok(pos, ",");
    do
    {
      switch(hash(pos, section_length(pos, next)))
      {
        case "defaults"_hash: break;

  // Generic
        case "ro"_hash          : mountflags |= MNT_RDONLY      ; break;
        case "rw"_hash          : mountflags &= 0^MNT_RDONLY    ; break;
        case "noexec"_hash      : mountflags |= MNT_NOEXEC      ; break;
        case "nosuid"_hash      : mountflags |= MNT_NOSUID      ; break;
        case "nodev"_hash       : mountflags |= MNT_NODEV       ; break;
        case "sync"_hash        : mountflags |= MNT_SYNCHRONOUS ; break;
        case "nodiratime"_hash  : mountflags |= MS_NODIRATIME   ; break;
        case "move"_hash        : mountflags |= MS_MOVE         ; break;
        case "bind"_hash        : mountflags |= MS_BIND         ; break;
        case "remount"_hash     : mountflags |= MS_REMOUNT      ; break;
        case "mandlock"_hash    : mountflags |= MS_MANDLOCK     ; break;
        case "dirsync"_hash     : mountflags |= MS_DIRSYNC      ; break;
        case "rec"_hash         : mountflags |= MS_REC          ; break;
        case "silent"_hash      : mountflags |= MS_SILENT       ; break;
        case "posixacl"_hash    : mountflags |= MS_POSIXACL     ; break;
        case "unbindable"_hash  : mountflags |= MS_UNBINDABLE   ; break;
        case "private"_hash     : mountflags |= MS_PRIVATE      ; break;
        case "slave"_hash       : mountflags |= MS_SLAVE        ; break;
        case "shared"_hash      : mountflags |= MS_SHARED       ; break;
        case "kernmount"_hash   : mountflags |= MS_KERNMOUNT    ; break;
        case "iversion"_hash    : mountflags |= MS_I_VERSION    ; break;
        case "strictatime"_hash : mountflags |= MS_STRICTATIME  ; break;
        case "lazytime"_hash    : mountflags |= MS_LAZYTIME     ; break;
        case "active"_hash      : mountflags |= MS_ACTIVE       ; break;
        case "nouser"_hash      : mountflags |= MS_NOUSER       ; break;
        case "union"_hash       : mountflags |= MNT_UNION       ; break;
        case "hidden"_hash      : mountflags |= MNT_HIDDEN      ; break;
        case "nocoredump"_hash  : mountflags |= MNT_NOCOREDUMP  ; break;
        case "noatime"_hash     : mountflags |= MNT_NOATIME     ; break;
        case "relatime"_hash    : mountflags |= MNT_RELATIME    ; break;
        case "norelatime"_hash  : mountflags &= 0^MNT_RELATIME  ; break;
        case "nodevmtime"_hash  : mountflags |= MNT_NODEVMTIME  ; break;
        case "symperm"_hash     : mountflags |= MNT_SYMPERM     ; break;
        case "async"_hash       : mountflags |= MNT_ASYNC       ; break;
        case "log"_hash         : mountflags |= MNT_LOG         ; break;
        case "extattr"_hash     : mountflags |= MNT_EXTATTR     ; break;
        case "snapshot"_hash    : mountflags |= MNT_SNAPSHOT    ; break;
        case "suiddir"_hash     : mountflags |= MNT_SUIDDIR     ; break;
        case "noclusterr"_hash  : mountflags |= MNT_NOCLUSTERR  ; break;
        case "noclusterw"_hash  : mountflags |= MNT_NOCLUSTERW  ; break;
        case "softdep"_hash     : mountflags |= MNT_SOFTDEP     ; break;
        case "wxallowed"_hash   : mountflags |= MNT_WXALLOWED   ; break;
        default:
  #if defined(__linux__)
          optionlist.append(pos, section_length(pos, next));
          if(next != NULL)
              optionlist.append(1, ',');
  #elif defined(__FreeBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(6,0,0)

  #else
  // NetBSD
          MOUNT_FFS
           struct ufs_args {
            char	   *fspec;	       /* block	special	file to	mount */
           };

          MOUNT_NFS
           struct nfs_args {
            int		 version;      /* args structure version */
            struct	sockaddr *addr;	       /* file server address */
            int		 addrlen;      /* length of address */
            int		 sotype;       /* Socket type */
            int		 proto;	       /* and Protocol */
            u_char		 *fh;	       /* File handle to be mounted */
            int		 fhsize;       /* Size,	in bytes, of fh	*/
            int		 flags;	       /* flags	*/
            int		 wsize;	       /* write	size in	bytes */
            int		 rsize;	       /* read size in bytes */
            int		 readdirsize;  /* readdir size in bytes	*/
            int		 timeo;	       /* initial timeout in .1	secs */
            int		 retrans;      /* times	to retry send */
            int		 maxgrouplist; /* Max. size of group list */
            int		 readahead;    /* # of blocks to readahead */
            int		 leaseterm;    /* Term (sec) of	lease */
            int		 deadthresh;   /* Retrans threshold */
            char		 *hostname;    /* server's name	*/
           };

          MOUNT_MFS
           struct mfs_args {
            char	   *fspec;	       /* name to export for statfs */
            struct	   export_args30 pad;  /* unused */
            caddr_t   base;	       /* base of file system in mem */
            u_long	   size;	       /* size of file system */
           };



  // Open BSD
           MOUNT_CD9660
            struct iso_args {
                char	   *fspec;     /* block	special	device to mount	*/
                struct	   export_args export_info;
                         /* network export info */
                int flags;	       /* mounting flags, see below */
            };
            #define ISOFSMNT_NORRIP   0x00000001	/* disable Rock	Ridge Ext.*/
            #define ISOFSMNT_GENS     0x00000002	/* enable generation numbers */
            #define ISOFSMNT_EXTATT   0x00000004	/* enable extended attributes */
            #define ISOFSMNT_NOJOLIET 0x00000008	/* disable Joliet Ext.*/
            #define ISOFSMNT_SESS     0x00000010	/* use iso_args.sess */

           MOUNT_FFS
            struct ufs_args {
             char	   *fspec;	       /* block	special	file to	mount */
             struct	   export_args export_info;
                            /* network export information */
            };

           MOUNT_MFS
            struct mfs_args {
             char	   *fspec;	       /* name to export for statfs */
             struct	   export_args export_info;
                            /* if we	can export an MFS */
             caddr_t   base;	       /* base of filesystem in	mem */
             u_long	   size;	       /* size of filesystem */
            };

           MOUNT_MSDOS
            struct msdosfs_args {
               char	   *fspec;    /* blocks	special	holding	fs to mount */
               struct  export_args export_info;
                        /* network export	information */
               uid_t   uid;	      /* uid that owns msdosfs files */
               gid_t   gid;	      /* gid that owns msdosfs files */
               mode_t  mask;      /* mask to be applied for	msdosfs	perms */
               int	   flags;     /* see below */
            };

            /*
             * Msdosfs mount options:
             */
            #define MSDOSFSMNT_SHORTNAME	1  /* Force old	DOS short names	only */
            #define MSDOSFSMNT_LONGNAME	2  /* Force Win'95 long	names */
            #define MSDOSFSMNT_NOWIN95	4  /* Completely ignore	Win95 entries */

           MOUNT_NFS
            struct nfs_args {
             int	   version;	   /* args structure version */
             struct	sockaddr *addr;	   /* file server address */
             int	   addrlen;	   /* length of	address	*/
             int	   sotype;	   /* Socket type */
             int	   proto;	   /* and Protocol */
             u_char	   *fh;		   /* File handle to be	mounted	*/
             int	   fhsize;	   /* Size, in bytes, of fh */
             int	   flags;	   /* flags */
             int	   wsize;	   /* write size in bytes */
             int	   rsize;	   /* read size	in bytes */
             int	   readdirsize;	   /* readdir size in bytes */
             int	   timeo;	   /* initial timeout in .1 secs */
             int	   retrans;	   /* times to retry send */
             int	   maxgrouplist;   /* Max. size	of group list */
             int	   readahead;	   /* #	of blocks to readahead */
             int	   leaseterm;	   /* Term (sec) of lease */
             int	   deadthresh;	   /* Retrans threshold	*/
             char	   *hostname;	   /* server's name */
             int	   acregmin;	 /* Attr cache file recently modified */
             int	   acregmax;	   /* ac file not recently modified */
             int	   acdirmin;	   /* ac for dir recently modified */
             int	   acdirmax;	 /* ac for dir not recently modified */
            };

           MOUNT_NTFS
            struct ntfs_args {
               char	   *fspec; /* block special device to mount */
               struct  export_args export_info;
                     /* network export information */
               uid_t   uid;	   /* uid that owns ntfs files */
               gid_t   gid;	   /* gid that owns ntfs files */
               mode_t  mode;   /* mask to be applied for ntfs perms	*/
               u_long  flag;   /* additional flags */
            };

            /*
             * ntfs mount options:
             */
            #define     NTFS_MFLAG_CASEINS      0x00000001
            #define     NTFS_MFLAG_ALLNAMES     0x00000002

           MOUNT_UDF
            struct udf_args {
               char	   *fspec; /* block special device to mount */
            };

  #endif
          break;
      }
      pos = next;
      next = std::strtok(NULL, ",");
    } while(pos != NULL && ++pos);
  }

  //MNT_UPDATE, MNT_RELOAD, and MNT_GETARGS

  for(const std::string& fs : fslist)
  {
#if defined(__linux__)
    if(mount(device, path, fs.c_str(), mountflags, optionlist.c_str()) == posix::success_response)
      return true;
#else

# if defined(__NetBSD__) && KERNEL_VERSION_CODE < KERNEL_VERSION(5,0,0)
#else
#endif

#endif
  }

  return false;
}

bool unmount(const char* path, const char* options) noexcept
{
#if defined(__linux__) && KERNEL_VERSION_CODE < KERNEL_VERSION(2,1,116)
  return umount(path) != posix::success_response;
#else
  int flags = 0;

  if(options != nullptr)
  {
    char* pos = const_cast<char*>(options);
    char* next = std::strtok(pos, ",");
    do
    {
      switch(hash(pos, section_length(pos, next)))
      {
        case "detach"_hash  : flags |= MNT_DETACH      ; break;
        case "force"_hash   : flags |= MNT_FORCE       ; break;
        case "expire"_hash  : flags |= MNT_EXPIRE      ; break;
        case "nofollow"_hash: flags |= UMOUNT_NOFOLLOW ; break;
      }
      pos = next;
      next = std::strtok(NULL, ",");
    } while(pos != NULL && ++pos);
  }

# if defined(__linux__)
  return umount2(path, flags) != posix::success_response;
# else
  return unmount(path, flags) != posix::success_response;
# endif
#endif
}
