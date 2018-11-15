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

#define MAX_MOUNT_MEM 160

// common mount flags
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

// Linux flags
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

// BSD flags
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

// unmount flags
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
  { return end == NULL ? std::strlen(start) : posix::size_t(end - start - 1); }

#define BSD

#if defined(BSD)
template<typename T> bool null_flags_parser(T*,char*,char*) { return false; }
template<typename T> bool null_kv_parser(T*,char*,char*,char*,char*) { return false; }
template<typename T> void null_finalizer(T*) { }


template<typename T>
bool parse_arg_fspec(T* args, char* key_start, char* key_end, char* val_start, char*)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "fspec"_hash: args->fspec = val_start; return true;
  }
  return false;
}

#include <socket.h>

struct export_bsdargs
{
  int       ex_flags;   /* export related flags */
  uid_t     ex_root;    /* mapping for root uid */
  struct xucred
  {
     uid_t	cr_uid;			/* user id */
     gid_t	cr_gid;			/* group id */
     short	cr_ngroups;		/* number of groups */
     gid_t	cr_groups[16];	/* groups */
  } ex_anon;    /* mapping for anonymous user */
  sockaddr* ex_addr;    /* net address to which exported */
  int       ex_addrlen; /* and the net address length */
  sockaddr* ex_mask;    /* mask of valid bits in saddr */
  int       ex_masklen; /* and the smask length */
};

// Arguments to mount amigados filesystems.
struct adosfs_bsdargs
{
  char*  fspec; // blocks special holding the fs to mount
  export_bsdargs export_info; // network export information
  uid_t  uid;   // uid that owns adosfs files
  gid_t  gid;   // gid that owns adosfs files
  mode_t mask;  // mask to be applied for adosfs perms
};

bool parse_arg_kv_adosfs(adosfs_bsdargs* args,
                           char* key_start, char* key_end,
                           char* val_start, char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "uid"_hash  : args->uid   = uid_t (std::strtoul(val_start, &val_end, 10)); return true;
    case "gid"_hash  : args->gid   = gid_t (std::strtoul(val_start, &val_end, 10)); return true;
    case "mask"_hash : args->mask  = mode_t(std::strtoul(val_start, &val_end,  8)); return true;
  }
  return parse_arg_fspec(args, key_start, key_end, val_start, val_end);
}

auto parse_arg_flags_adosfs = null_flags_parser<adosfs_bsdargs>;
auto arg_finalize_adosfs    = null_finalizer   <adosfs_bsdargs>;

//--------------------------------------------------------
// Arguments to mount autofs filesystem.
struct autofs_bsdargs
{
  char* from;
  char* master_options;
  char* master_prefix;
};

//--------------------------------------------------------
// Arguments to mount ISO 9660 filesystems.
struct iso9660_bsdargs
{
  char* fspec;  // block special device to mount
  export_bsdargs export_info; // network export information
  int   flags;  // mounting flags, see below
};

// flags
#define BSD_ISOFSMNT_NORRIP       0x00000001 // disable Rock Ridge Ext.
#define BSD_ISOFSMNT_GENS         0x00000002 // enable generation numbers
#define BSD_ISOFSMNT_EXTATT       0x00000004 // enable extended attributes
#define BSD_ISOFSMNT_NOJOLIET     0x00000008 // disable Joliet extensions
#define BSD_ISOFSMNT_NOCASETRANS  0x00000010 // do not make names lower case
#define BSD_ISOFSMNT_RRCASEINS    0x00000020 // case insensitive Rock Ridge
#define BSD_ISOFSMNT_SESS         0x00000010 // use iso_args.sess

bool parse_arg_flags_iso9660(iso9660_bsdargs* args, char* start, char* end)
{
  switch(hash(start, section_length(start, end)))
  {
    case "norrip"_hash     : args->flags |= BSD_ISOFSMNT_NORRIP       ; return true;
    case "gens"_hash       : args->flags |= BSD_ISOFSMNT_GENS         ; return true;
    case "extatt"_hash     : args->flags |= BSD_ISOFSMNT_EXTATT       ; return true;
    case "nojoliet"_hash   : args->flags |= BSD_ISOFSMNT_NOJOLIET     ; return true;
#if defined(__NetBSD__)
    case "nocasetrans"_hash: args->flags |= BSD_ISOFSMNT_NOCASETRANS  ; return true;
    case "rrcaseins"_hash  : args->flags |= BSD_ISOFSMNT_RRCASEINS    ; return true;
#endif
  }
  return false;
}

#if defined(__OpenBSD__)
bool parse_arg_kv_iso9660(iso9660_bsdargs* args,
                          char* key_start, char* key_end,
                          char* val_start, char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "session"_hash:
      args->sess = int(std::strtol(val_start, &val_end, 10));
      args->flags |= BSD_ISOFSMNT_SESS;
      return true;
  }
  return false;
}
#else
auto parse_arg_kv_iso9660 = null_kv_parser<iso9660_bsdargs>;
#endif
auto arg_finalize_iso9660 = null_finalizer<iso9660_bsdargs>;


//--------------------------------------------------------

struct ufs_bsdargs
{
  char*  fspec;    // block special device to mount */
  export_bsdargs export_info; // network export information
};

auto parse_arg_kv_ufs    = parse_arg_fspec  <ufs_bsdargs>;
auto parse_arg_flags_ufs = null_flags_parser<ufs_bsdargs>;
auto arg_finalize_ufs    = null_finalizer   <ufs_bsdargs>;

//--------------------------------------------------------
struct efs_bsdargs
{
  char* fspec;   // block special device to mount
  int   version;
};

//--------------------------------------------------------
// Arguments to mount Acorn Filecore filesystems.
struct filecore_bsdargs
{
  char* fspec; // block special device to mount
  export_bsdargs export_info; // network export information
  uid_t uid;   // uid that owns filecore files
  gid_t gid;   // gid that owns filecore files
  int   flags; // mounting flags, see below
};

// flags
#define BSD_FILECOREMNT_ROOT      0x00000000
#define BSD_FILECOREMNT_OWNACCESS 0x00000001 // Only user has Owner access
#define BSD_FILECOREMNT_ALLACCESS 0x00000002 // World has Owner access
#define BSD_FILECOREMNT_OWNREAD   0x00000004 // All files have Owner read access
#define BSD_FILECOREMNT_USEUID    0x00000008 // Use uid of mount process
#define BSD_FILECOREMNT_FILETYPE  0x00000010 // Include filetype in filename


bool parse_arg_kv_filecore(filecore_bsdargs* args,
                           char* key_start, char* key_end,
                           char* val_start, char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "uid"_hash  : args->uid = uid_t (std::strtoul(val_start, &val_end, 10)); return true;
    case "gid"_hash  : args->gid = gid_t (std::strtoul(val_start, &val_end, 10)); return true;
  }
  return parse_arg_fspec(args, key_start, key_end, val_start, val_end);
}

bool parse_arg_flags_filecore(filecore_bsdargs* args, char* start, char* end)
{
  switch(hash(start, section_length(start, end)))
  {
    case "root"_hash      : args->flags |= BSD_FILECOREMNT_ROOT     ; return true;
    case "ownaccess"_hash : args->flags |= BSD_FILECOREMNT_OWNACCESS; return true;
    case "allaccess"_hash : args->flags |= BSD_FILECOREMNT_ALLACCESS; return true;
    case "ownread"_hash   : args->flags |= BSD_FILECOREMNT_OWNREAD  ; return true;
    case "useuid"_hash    : args->flags |= BSD_FILECOREMNT_USEUID   ; return true;
    case "filetype"_hash  : args->flags |= BSD_FILECOREMNT_FILETYPE ; return true;
  }
  return false;
}

auto arg_finalize_filecore = null_finalizer<filecore_bsdargs>;

//--------------------------------------------------------
struct hfs_bsdargs
{
  char* fspec;  // block special device to mount
};

auto parse_arg_kv_hfs    = parse_arg_fspec  <hfs_bsdargs>;
auto parse_arg_flags_hfs = null_flags_parser<hfs_bsdargs>;
auto arg_finalize_hfs    = null_finalizer   <hfs_bsdargs>;

//--------------------------------------------------------
// Arguments to mount MSDOS filesystems.
struct msdosfs_bsdargs
{
  char*  fspec; // blocks special holding the fs to mount
  export_bsdargs export_info; // network export information
  uid_t  uid;   // uid that owns msdosfs files
  gid_t  gid;   // gid that owns msdosfs files
  mode_t mask;  // mask to be applied for msdosfs perms
  int    flags; // see below

  // Following items added after versioning support
  int    version; // version of the struct
  mode_t dirmask; // v2: mask to be applied for msdosfs perms
  int    gmtoff;  // v3: offset from UTC in seconds
};

// flags
#define BSD_MSDOSFSMNT_SHORTNAME  0x00000001 // Force old DOS short names only
#define BSD_MSDOSFSMNT_LONGNAME   0x00000002 // Force Win'95 long names
#define BSD_MSDOSFSMNT_NOWIN95    0x00000004 // Completely ignore Win95 entries
#define BSD_MSDOSFSMNT_GEMDOSFS   0x00000008 // This is a GEMDOS-flavour
#define BSD_MSDOSFSMNT_VERSIONED  0x00000010 // Struct is versioned
#define BSD_MSDOSFSMNT_UTF8       0x00000020 // Use UTF8 filenames
#define BSD_MSDOSFSMNT_RONLY      0x80000000 // mounted read-only
#define BSD_MSDOSFSMNT_WAITONFAT  0x40000000 // mounted synchronous
#define BSD_MSDOSFS_FATMIRROR     0x20000000 // FAT is mirrored

bool parse_arg_kv_msdosfs(msdosfs_bsdargs* args,
                           char* key_start, char* key_end,
                           char* val_start, char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "uid"_hash     : args->uid     = uid_t (std::strtoul(val_start, &val_end, 10)); return true;
    case "gid"_hash     : args->gid     = gid_t (std::strtoul(val_start, &val_end, 10)); return true;
    case "mask"_hash    : args->mask    = mode_t(std::strtoul(val_start, &val_end,  8)); return true;
    case "dirmask"_hash : args->dirmask = mode_t(std::strtoul(val_start, &val_end,  8)); args->version |= 2; return true;
    case "gmtoff"_hash  : args->gmtoff  = int   (std::strtol (val_start, &val_end, 10)); args->version |= 3; return true;
  }
  return parse_arg_fspec(args, key_start, key_end, val_start, val_end);
}

bool parse_arg_flags_msdosfs(msdosfs_bsdargs* args, char* start, char* end)
{
  switch(hash(start, section_length(start, end)))
  {
    case "shortname"_hash   : args->flags |= BSD_MSDOSFSMNT_SHORTNAME ; return true;
    case "longname"_hash    : args->flags |= BSD_MSDOSFSMNT_LONGNAME  ; return true;
    case "nowin95"_hash     : args->flags |= BSD_MSDOSFSMNT_NOWIN95   ; return true;
    case "gemdosfs"_hash    : args->flags |= BSD_MSDOSFSMNT_GEMDOSFS  ; return true;
    case "mntversioned"_hash: args->flags |= BSD_MSDOSFSMNT_VERSIONED ; return true;
    case "utf8"_hash        : args->flags |= BSD_MSDOSFSMNT_UTF8      ; return true;
    case "ronly"_hash       : args->flags |= BSD_MSDOSFSMNT_RONLY     ; return true;
    case "waitonfat"_hash   : args->flags |= BSD_MSDOSFSMNT_WAITONFAT ; return true;
    case "fatmirror"_hash   : args->flags |= BSD_MSDOSFS_FATMIRROR    ; return true;
  }
  return false;
}

void arg_finalize_msdosfs(msdosfs_bsdargs* data)
{
  if(data->dirmask != 0)
    data->version = 2;
  if(data->gmtoff != 0)
    data->version = 3;
}


//--------------------------------------------------------
// Arguments to mount NILFS filingsystem.
struct nilfs_bsdargs
{
  uint32_t version;     // version of this structure
  char*    fspec;       // mount specifier
  uint32_t nilfsmflags; // mount options

  int32_t  gmtoff;      // offset from UTC in seconds
  int64_t  cpno;        // checkpoint number

  // extendable
  uint8_t  reserved[32];
};

void arg_finalize_nilfs(nilfs_bsdargs* data)
{
  data->version = 1;
}

//--------------------------------------------------------
struct ntfs_bsdargs
{
  char*  fspec; // block special device to mount
  export_bsdargs export_info; // network export information
  uid_t  uid;   // uid that owns ntfs files
  gid_t  gid;   // gid that owns ntfs files
  mode_t mask;  // mask to be applied for ntfs perms
  u_long flags;  // additional flags
};

// flags
#define BSD_NTFS_MFLAG_CASEINS  0x00000001
#define BSD_NTFS_MFLAG_ALLNAMES 0x00000002

bool parse_arg_kv_ntfs(ntfs_bsdargs* args,
                       char* key_start, char* key_end,
                       char* val_start, char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "uid"_hash  : args->uid   = uid_t (std::strtoul(val_start, &val_end, 10)); return true;
    case "gid"_hash  : args->gid   = gid_t (std::strtoul(val_start, &val_end, 10)); return true;
    case "mask"_hash : args->mask  = mode_t(std::strtoul(val_start, &val_end,  8)); return true;
  }
  return parse_arg_fspec(args, key_start, key_end, val_start, val_end);
}

bool parse_arg_flags_ntfs(ntfs_bsdargs* args, char* start, char* end)
{
  switch(hash(start, section_length(start, end)))
  {
    case "caseins"_hash : args->flags |= BSD_NTFS_MFLAG_CASEINS ; return true;
    case "allnames"_hash: args->flags |= BSD_NTFS_MFLAG_ALLNAMES; return true;
  }
  return false;
}

auto arg_finalize_ntfs = null_finalizer<ntfs_bsdargs>;

//--------------------------------------------------------
#define PTYFS_ARGSVERSION 2
struct ptyfs_bsdargs
{
  int    version;
  gid_t  gid;
  mode_t mask;
  int    flags;
};


bool parse_arg_kv_ptyfs(ptyfs_bsdargs* args,
                        char* key_start, char* key_end,
                        char* val_start, char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "gid"_hash  : args->gid   = gid_t (std::strtoul(val_start, &val_end, 10)); return true;
    case "mask"_hash : args->mask  = mode_t(std::strtoul(val_start, &val_end,  8)); return true;
  }
  return false;
}

bool parse_arg_flags_ptyfs(ptyfs_bsdargs* args, char* start, char* end)
{
  switch(hash(start, section_length(start, end)))
  {
    // TODO
    case "???"_hash   : args->flags |= 0 ; return true;
  }
  return false;
}


void arg_finalize_ptyfs(ptyfs_bsdargs* data)
{
  data->version = 1;
  if(data->gid != 0 || data->mask != 0) // TODO: verify
    data->version = 2;
}


//--------------------------------------------------------
// Layout of the mount control block for a netware file system.
struct smbfs_bsdargs
{
  int    version;
  int    dev_fd;  // descriptor of open nsmb device
  u_int  flags;
  uid_t  uid;
  gid_t  gid;
  mode_t file_mode;
  mode_t dir_mode;
  int    caseopt;
};

// flags
#define BSD_SMBFS_MOUNT_SOFT      0x0001
#define BSD_SMBFS_MOUNT_INTR      0x0002
#define BSD_SMBFS_MOUNT_STRONG    0x0004
#define BSD_SMBFS_MOUNT_HAVE_NLS  0x0008
#define BSD_SMBFS_MOUNT_NO_LONG   0x0010


//--------------------------------------------------------
struct sysvbfs_bsdargs
{
  char* fspec;  // blocks special holding the fs to mount
};

auto parse_arg_kv_sysvbfs    = parse_arg_fspec  <sysvbfs_bsdargs>;
auto parse_arg_flags_sysvbfs = null_flags_parser<sysvbfs_bsdargs>;
auto arg_finalize_sysvbfs    = null_finalizer   <sysvbfs_bsdargs>;

//--------------------------------------------------------
struct tmpfs_bsdargs
{
  int    version;

   // Size counters.
  ino_t  nodes_max;
  off_t  size_max;

   // Root node attributes.
  uid_t  root_uid;
  gid_t  root_gid;
  mode_t root_mode;
};

auto parse_arg_flags_tmpfs = null_flags_parser<tmpfs_bsdargs>;

void arg_finalize_tmpfs(tmpfs_bsdargs* data)
{
  data->version = 1;
}

//--------------------------------------------------------

// Arguments to mount UDF filingsystem.
struct udf_bsdargs
{
  uint32_t version;      // version of this structure
  char*    fspec;        // mount specifier
  int32_t  sessionnr;    // session specifier, rel of abs
  uint32_t flags;    // mount options
  int32_t  gmtoff;       // offset from UTC in seconds

  uid_t    anon_uid;     // mapping of anonymous files uid
  gid_t    anon_gid;     // mapping of anonymous files gid
  uid_t    nobody_uid;   // nobody:nobody will map to -1:-1
  gid_t    nobody_gid;   // nobody:nobody will map to -1:-1

  uint32_t sector_size;  // for mounting dumps/files

  // extendable
  uint8_t  reserved[32];
};

// udf mount options
#define BSD_UDFMNT_CLOSESESSION 0x00000001 // close session on dismount


void arg_finalize_udf(udf_bsdargs* data)
{
  data->version = 1;
}

//--------------------------------------------------------

struct union_bsdargs
{
  char* fspec; // Target of loopback
  int   flags;  // Options on the mount
};

// flags
#define BSD_UNMNT_ABOVE   0x0001  // Target appears below mount point
#define BSD_UNMNT_BELOW   0x0002  // Target appears below mount point
#define BSD_UNMNT_REPLACE 0x0003  // Target replaces mount point

auto parse_arg_kv_union = parse_arg_fspec<union_bsdargs>;

bool parse_arg_flags_union(union_bsdargs* args, char* start, char* end)
{
  switch(hash(start, section_length(start, end)))
  {
    case "above"_hash   : args->flags |= BSD_UNMNT_ABOVE  ; return true;
    case "below"_hash   : args->flags |= BSD_UNMNT_BELOW  ; return true;
    case "replace"_hash : args->flags |= BSD_UNMNT_REPLACE; return true;
  }
  return false;
}

auto arg_finalize_union = null_finalizer<union_bsdargs>;

//--------------------------------------------------------

struct v7fs_bsdargs
{
  char* fspec;  // blocks special holding the fs to mount
  int   endian; // target filesystem endian
};


//auto parse_arg_kv_v7fs   = parse_arg_fspec  <v7fs_bsdargs>;
auto parse_arg_flags_v7fs = null_flags_parser<v7fs_bsdargs>;
auto arg_finalize_v7fs    = null_finalizer   <v7fs_bsdargs>;


template<typename T>
using arg_finalize = void (*)(T*);
template<typename T>
using arg_parser_flag = bool (*)(T*, char*, char*);
template<typename T>
using arg_parser_kv = bool (*)(T*, char*, char*, char*, char*);

template<typename T>
bool parse_args(void* data, const std::string& options,
                arg_parser_flag<T> parse_flag,
                arg_parser_kv<T> parse_kv,
                arg_finalize<T> finalize)
{
  std::memset(data, 0, MAX_MOUNT_MEM);
  T* args = static_cast<T*>(data);
  char* pos = const_cast<char*>(options.c_str());
  char* next = std::strtok(pos, "=,");
  do
  {
    if(next == NULL || *next == ',')
    {
      if(!parse_flag(args, pos, next))
        return false;
      pos = next;
    }
    else
    {
      char* end = std::strtok(NULL, ",");
      if(!parse_kv(args, pos, next, next + 1, end))
        return false;
      pos = end;
    }

    next = std::strtok(NULL, "=,");
  } while(pos != NULL && ++pos);
  finalize(args);
  return true;
}


bool mount_bsd(const char* device,
               const char* path,
               const char* filesystem,
               const char* options,
                     void* data) noexcept;
#endif
bool mount(const char* device,
           const char* path,
           const char* filesystem,
           const char* options) noexcept
#if defined(BSD)
{
  void* data = ::malloc(MAX_MOUNT_MEM); // allocate memory
  if(data == NULL)
    return false;
  bool result = mount_bsd(device, path, filesystem, options, data);
  ::free(data);
  return result;
}

bool mount_bsd(const char* device,
               const char* path,
               const char* filesystem,
               const char* options,
                     void* data) noexcept
#endif
{
  std::list<std::string> fslist;
  char *pos, *next;
  int mountflags = 0;
  std::string optionlist;

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
// Linux Only
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
// BSD
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
          optionlist.append(pos, section_length(pos, next));
          if(next != NULL)
            optionlist.append(1, ',');
          break;
      }
      pos = next;
      next = std::strtok(NULL, ",");
    } while(pos != NULL && ++pos);
  }

  //MNT_UPDATE, MNT_RELOAD, and MNT_GETARGS

  for(const std::string& fs : fslist)
  {
#if 0 && defined(__linux__)
    typedef unsigned long mnt_flag_t; // defined for mount()
    if(::mount(device, path, fs.c_str(), mnt_flag_t(mountflags), optionlist.c_str()) == posix::success_response)
      return true;
#elif defined(BSD)
# define CASE_CONCAT(a, b) a##b
# define CASE_QUOTE(x) #x
# define PARSE_ARG_CASE(x) \
    case compiletime_hash(CASE_QUOTE(x), sizeof(CASE_QUOTE(x)) - 1) : \
      parse_ok = parse_args(data, optionlist, \
                            CASE_CONCAT(parse_arg_flags_, x), \
                            CASE_CONCAT(parse_arg_kv_, x), \
                            CASE_CONCAT(arg_finalize_, x)); \
      break

    bool parse_ok = false;
    if(device != nullptr)
      optionlist.append("fspec=").append(device);
    switch(hash(fs))
    {
      case "cd9660"_hash:
      PARSE_ARG_CASE(iso9660);

      case "msdos"_hash:
      PARSE_ARG_CASE(msdosfs);

      case "ffs"_hash:
      PARSE_ARG_CASE(ufs);

      PARSE_ARG_CASE(adosfs);
      PARSE_ARG_CASE(filecore);
      PARSE_ARG_CASE(ntfs);
      PARSE_ARG_CASE(hfs);
      PARSE_ARG_CASE(ptyfs);
      PARSE_ARG_CASE(sysvbfs);
      PARSE_ARG_CASE(union);
      PARSE_ARG_CASE(autofs);
      PARSE_ARG_CASE(efs);
      PARSE_ARG_CASE(nilfs);
      PARSE_ARG_CASE(smbfs);
      PARSE_ARG_CASE(tmpfs);
      PARSE_ARG_CASE(udf);
      PARSE_ARG_CASE(nfs);
      PARSE_ARG_CASE(mfs);
      PARSE_ARG_CASE(afs);
      PARSE_ARG_CASE(fuse);
      PARSE_ARG_CASE(ext2fs);
      PARSE_ARG_CASE(ncpfs);
    }
    if(parse_ok)
    {
# if defined(__NetBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(5,0,0)
      if(::mount(fs.c_str(), path, mountflags, data, MAX_MOUNT_MEM) == posix::success_response)
        return true;
# else
      if(::mount(fs.c_str(), path, mountflags, data) == posix::success_response)
        return true;
# endif
    }
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
