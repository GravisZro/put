#include "mount.h"

// POSIX
#include <assert.h>

// POSIX
#include <put/socket.h>

// PUT
#include <put/cxxutils/posix_helpers.h>
#include <put/cxxutils/hashing.h>
#include <put/cxxutils/stringtoken.h>
#include <put/specialized/osdetect.h>
#include <put/specialized/fstable.h>

// STL
#include <list>
#include <string>


#if defined(__ultrix__)
# include <sys/fs_types.h>
#else
// Linux/*BSD/Darwin
# include <sys/mount.h>
#endif

#define MAX_MOUNT_MEM 160

// common mount flags
#if defined(__linux__)
# define MNT_RDONLY       MS_RDONLY
# define MNT_NOEXEC       MS_NOEXEC
# define MNT_NOSUID       MS_NOSUID
# define MNT_NODEV        MS_NODEV
# define MNT_SYNCHRONOUS  MS_SYNCHRONOUS
# define MNT_NOATIME      MS_NOATIME
# define MNT_RELATIME     MS_RELATIME
#elif defined(BSD) && !defined(MNT_RDONLY)
# define MNT_RDONLY       M_RDONLY
# define MNT_NOEXEC       M_NOEXEC
# define MNT_NOSUID       M_NOSUID
# define MNT_NODEV        M_NODEV
# define MNT_SYNCHRONOUS  M_SYNCHRONOUS
# define MNT_RDONLY       M_RDONLY
#endif

// Linux flags
#ifndef MS_NODIRATIME
# define MS_NODIRATIME    0
#endif
#ifndef MS_MOVE
# define MS_MOVE          0
#endif
#ifndef MS_BIND
# define MS_BIND          0
#endif
#ifndef MS_REMOUNT
# define MS_REMOUNT       0
#endif
#ifndef MS_MANDLOCK
# define MS_MANDLOCK      0
#endif
#ifndef MS_DIRSYNC
# define MS_DIRSYNC       0
#endif
#ifndef MS_REC
# define MS_REC           0
#endif
#ifndef MS_SILENT
# define MS_SILENT        0
#endif
#ifndef MS_POSIXACL
# define MS_POSIXACL      0
#endif
#ifndef MS_UNBINDABLE
# define MS_UNBINDABLE    0
#endif
#ifndef MS_PRIVATE
# define MS_PRIVATE       0
#endif
#ifndef MS_SLAVE
# define MS_SLAVE         0
#endif
#ifndef MS_SHARED
# define MS_SHARED        0
#endif
#ifndef MS_KERNMOUNT
# define MS_KERNMOUNT     0
#endif
#ifndef MS_I_VERSION
# define MS_I_VERSION     0
#endif
#ifndef MS_STRICTATIME
# define MS_STRICTATIME   0
#endif
#ifndef MS_LAZYTIME
# define MS_LAZYTIME      0
#endif
#ifndef MS_ACTIVE
# define MS_ACTIVE        0
#endif
#ifndef MS_NOUSER
# define MS_NOUSER        0
#endif

// BSD flags
#ifndef MNT_UNION
# define MNT_UNION        0
#endif
#ifndef MNT_HIDDEN
# define MNT_HIDDEN       0
#endif
#ifndef MNT_NOCOREDUMP
# define MNT_NOCOREDUMP   0
#endif
#ifndef MNT_NOATIME
# define MNT_NOATIME      0
#endif
#ifndef MNT_RELATIME
# define MNT_RELATIME     0
#endif
#ifndef MNT_NODEVMTIME
# define MNT_NODEVMTIME   0
#endif
#ifndef MNT_SYMPERM
# define MNT_SYMPERM      0
#endif
#ifndef MNT_ASYNC
# define MNT_ASYNC        0
#endif
#ifndef MNT_LOG
# define MNT_LOG          0
#endif
#ifndef MNT_EXTATTR
# define MNT_EXTATTR      0
#endif
#ifndef MNT_SNAPSHOT
# define MNT_SNAPSHOT     0
#endif
#ifndef MNT_SUIDDIR
# define MNT_SUIDDIR      0
#endif
#ifndef MNT_NOCLUSTERR
# define MNT_NOCLUSTERR   0
#endif
#ifndef MNT_NOCLUSTERW
# define MNT_NOCLUSTERW   0
#endif
#ifndef MNT_EXTATTR
# define MNT_EXTATTR      0
#endif
#ifndef MNT_SOFTDEP
# define MNT_SOFTDEP      0
#endif
#ifndef MNT_WXALLOWED
# define MNT_WXALLOWED    0
#endif

// unmount flags
#ifndef MNT_FORCE
# define MNT_FORCE        0
#endif
#ifndef MNT_DETACH
# define MNT_DETACH       0
#endif
#ifndef MNT_EXPIRE
# define MNT_EXPIRE       0
#endif
#ifndef UMOUNT_NOFOLLOW
# define UMOUNT_NOFOLLOW  0
#endif

// BSD filesystem types
#ifndef MOUNT_UFS
# define MOUNT_UFS        0
#endif
#ifndef MOUNT_NFS
# define MOUNT_NFS        0
#endif
#ifndef MOUNT_MFS
# define MOUNT_MFS        0
#endif
#ifndef MOUNT_MSDOS
# define MOUNT_MSDOS      0
#endif
#ifndef MOUNT_LFS
# define MOUNT_LFS        0
#endif
#ifndef MOUNT_LOFS
# define MOUNT_LOFS       0
#endif
#ifndef MOUNT_FDESC
# define MOUNT_FDESC      0
#endif
#ifndef MOUNT_PORTAL
# define MOUNT_PORTAL     0
#endif
#ifndef MOUNT_NULL
# define MOUNT_NULL       0
#endif
#ifndef MOUNT_UMAP
# define MOUNT_UMAP       0
#endif
#ifndef MOUNT_KERNFS
# define MOUNT_KERNFS     0
#endif
#ifndef MOUNT_PROCFS
# define MOUNT_PROCFS     0
#endif
#ifndef MOUNT_AFS
# define MOUNT_AFS        0
#endif
#ifndef MOUNT_CD9660
# define MOUNT_CD9660     0
#endif
#ifndef MOUNT_UNION
# define MOUNT_UNION      0
#endif
#ifndef MOUNT_DEVFS
# define MOUNT_DEVFS      0
#endif
#ifndef MOUNT_EXT2FS
# define MOUNT_EXT2FS     0
#endif
#ifndef MOUNT_TFS
# define MOUNT_TFS        0
#endif
#ifndef MOUNT_CFS
# define MOUNT_CFS        0
#endif

constexpr posix::size_t section_length(const char* start, const char* end)
  { return end == nullptr ? posix::strlen(start) : posix::size_t(end - start - 1); }

//#define BSD

#if defined(BSD)
template<typename T> bool null_flags_parser(T*,const char*,const char*) { return false; }
template<typename T> bool null_kv_parser(T*,const char*,const char*,const char*,const char*) { return false; }
template<typename T> void null_finalizer(T*) { }

template<typename T, int base>
T decode(const char* start, const char* end)
{
  static_assert(base <= 10, "not supported");
  T value = 0;
  bool neg = *start == '-';
  if(neg)
    ++start;
  for(const char* pos = start; pos != end && posix::isdigit(*pos); ++pos)
  {
    value *= base;
    value += *pos - '0';
  }
  if(neg)
    value *= -1;
  return value;
}

template<typename T>
bool parse_arg_fspec(T* args, const char* key_start, const char* key_end, const char* val_start, const char*)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "fspec"_hash: args->fspec = val_start; return true;
  }
  return false;
}

struct export_bsdargs
{
  int       ex_flags;   // export related flags
  uid_t     ex_root;    // mapping for root uid
  struct xucred_t
  {
     uid_t ex_uid;   // user id
     gid_t ex_gid;   // group id
     short ex_ngroups;  // number of groups
     gid_t ex_groups[16]; // groups
  } ex_anon;            // mapping for anonymous user
  sockaddr* ex_addr;    // net address to which exported
  int       ex_addrlen; // and the net address length
  sockaddr* ex_mask;    // mask of valid bits in saddr
  int       ex_masklen; // and the smask length
};

// Arguments to mount amigados filesystems.
struct adosfs_bsdargs
{
  const char*    fspec;       // block special device to mount
  export_bsdargs export_info; // network export information
  uid_t  uid;   // uid that owns adosfs files
  gid_t  gid;   // gid that owns adosfs files
  mode_t mask;  // mask to be applied for adosfs perms
};

bool parse_arg_kv_adosfs(adosfs_bsdargs* args,
                         const char* key_start, const char* key_end,
                         const char* val_start, const char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "uid"_hash  : args->uid  = decode<uid_t, 10>(val_start, val_end); return true;
    case "gid"_hash  : args->gid  = decode<gid_t, 10>(val_start, val_end); return true;
    case "mask"_hash : args->mask = decode<mode_t, 8>(val_start, val_end); return true;
  }
  return parse_arg_fspec(args, key_start, key_end, val_start, val_end);
}

auto parse_arg_flags_adosfs = null_flags_parser<adosfs_bsdargs>;
auto arg_finalize_adosfs    = null_finalizer   <adosfs_bsdargs>;

//--------------------------------------------------------
// Arguments to mount autofs filesystem.
struct autofs_bsdargs
{
  const char* from;
  const char* master_options;
  const char* master_prefix;
};

//--------------------------------------------------------
// Arguments to mount ISO 9660 filesystems.
struct cd9660_bsdargs
{
  const char*    fspec;       // block special device to mount
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

bool parse_arg_flags_cd9660(cd9660_bsdargs* args, const char* start, const char* end)
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
bool parse_arg_kv_cd9660(cd9660_bsdargs* args,
                         const char* key_start, const char* key_end,
                         const char* val_start, const char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "session"_hash:
      args->sess = decode<int, 10>(val_start, val_end);
      args->flags |= BSD_ISOFSMNT_SESS;
      return true;
  }
  return false;
}
#else
auto parse_arg_kv_cd9660 = null_kv_parser<cd9660_bsdargs>;
#endif
auto arg_finalize_cd9660 = null_finalizer<cd9660_bsdargs>;


//--------------------------------------------------------

struct ufs_bsdargs
{
  const char*    fspec;       // block special device to mount
  export_bsdargs export_info; // network export information
};

auto parse_arg_kv_ufs    = parse_arg_fspec  <ufs_bsdargs>;
auto parse_arg_flags_ufs = null_flags_parser<ufs_bsdargs>;
auto arg_finalize_ufs    = null_finalizer   <ufs_bsdargs>;

//--------------------------------------------------------
struct efs_bsdargs
{
  const char* fspec;   // block special device to mount
  int   version;
};

//--------------------------------------------------------
// Arguments to mount Acorn Filecore filesystems.
struct filecore_bsdargs
{
  const char*    fspec;       // block special device to mount
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
                           const char* key_start, const char* key_end,
                           const char* val_start, const char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "uid"_hash  : args->uid = decode<uid_t, 10>(val_start, val_end); return true;
    case "gid"_hash  : args->gid = decode<gid_t, 10>(val_start, val_end); return true;
  }
  return parse_arg_fspec(args, key_start, key_end, val_start, val_end);
}

bool parse_arg_flags_filecore(filecore_bsdargs* args, const char* start, const char* end)
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
  const char* fspec;  // block special device to mount
};

auto parse_arg_kv_hfs    = parse_arg_fspec  <hfs_bsdargs>;
auto parse_arg_flags_hfs = null_flags_parser<hfs_bsdargs>;
auto arg_finalize_hfs    = null_finalizer   <hfs_bsdargs>;

//--------------------------------------------------------
// Arguments to mount MSDOS filesystems.
struct msdosfs_bsdargs
{
  const char*    fspec;       // block special device to mount
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
                           const char* key_start, const char* key_end,
                           const char* val_start, const char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "uid"_hash     : args->uid     = decode<uid_t, 10>(val_start, val_end); return true;
    case "gid"_hash     : args->gid     = decode<gid_t, 10>(val_start, val_end); return true;
    case "mask"_hash    : args->mask    = decode<mode_t, 8>(val_start, val_end); return true;
    case "dirmask"_hash : args->dirmask = decode<mode_t, 8>(val_start, val_end); args->version |= 2; return true;
    case "gmtoff"_hash  : args->gmtoff  = decode<int,   10>(val_start, val_end); args->version |= 3; return true;
  }
  return parse_arg_fspec(args, key_start, key_end, val_start, val_end);
}

bool parse_arg_flags_msdosfs(msdosfs_bsdargs* args, const char* start, const char* end)
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
  const char* fspec;       // mount specifier
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
  const char*    fspec;       // block special device to mount
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
                       const char* key_start, const char* key_end,
                       const char* val_start, const char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "uid"_hash  : args->uid   = decode<uid_t, 10>(val_start, val_end); return true;
    case "gid"_hash  : args->gid   = decode<gid_t, 10>(val_start, val_end); return true;
    case "mask"_hash : args->mask  = decode<mode_t, 8>(val_start, val_end); return true;
  }
  return parse_arg_fspec(args, key_start, key_end, val_start, val_end);
}

bool parse_arg_flags_ntfs(ntfs_bsdargs* args, const char* start, const char* end)
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
struct ptyfs_bsdargs
{
  int    version;
  gid_t  gid;
  mode_t mask;
  int    flags;
};

bool parse_arg_kv_ptyfs(ptyfs_bsdargs* args,
                        const char* key_start, const char* key_end,
                        const char* val_start, const char* val_end)
{
  switch(hash(key_start, section_length(key_start, key_end)))
  {
    case "gid"_hash  : args->gid   = decode<gid_t, 10>(val_start, val_end); return true;
    case "mask"_hash : args->mask  = decode<mode_t, 8>(val_start, val_end); return true;
  }
  return false;
}

bool parse_arg_flags_ptyfs(ptyfs_bsdargs* args, const char* start, const char* end)
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
  const char* fspec;  // blocks special holding the fs to mount
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
  const char* fspec;        // mount specifier
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
  const char* fspec; // Target of loopback
  int   flags;  // Options on the mount
};

// flags
#define BSD_UNMNT_ABOVE   0x0001  // Target appears below mount point
#define BSD_UNMNT_BELOW   0x0002  // Target appears below mount point
#define BSD_UNMNT_REPLACE 0x0003  // Target replaces mount point

auto parse_arg_kv_union = parse_arg_fspec<union_bsdargs>;

bool parse_arg_flags_union(union_bsdargs* args, const char* start, const char* end)
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
  const char* fspec;  // blocks special holding the fs to mount
  int   endian; // target filesystem endian
};


//auto parse_arg_kv_v7fs   = parse_arg_fspec  <v7fs_bsdargs>;
auto parse_arg_flags_v7fs = null_flags_parser<v7fs_bsdargs>;
auto arg_finalize_v7fs    = null_finalizer   <v7fs_bsdargs>;


template<typename T>
using arg_finalize = void (*)(T*);
template<typename T>
using arg_parser_flag = bool (*)(T*, const char*, const char*);
template<typename T>
using arg_parser_kv = bool (*)(T*, const char*, const char*, const char*, const char*);

template<typename T>
bool parse_args(void* data, const std::string& options,
                arg_parser_flag<T> parse_flag,
                arg_parser_kv<T> parse_kv,
                arg_finalize<T> finalize)
{
  posix::memset(data, 0, MAX_MOUNT_MEM);
  T* args = static_cast<T*>(data);
  StringToken tok(options.c_str());
  const char* pos = tok.pos();
  const char* next = tok.next("=,");
  do
  {
    if(next == nullptr || *next == ',')
    {
      if(!parse_flag(args, pos, next))
        return false;
      pos = next;
    }
    else
    {
      const char* end = tok.next(',');
      if(!parse_kv(args, pos, next, next + 1, end))
        return false;
      pos = end;
    }

    next = tok.next("=,");
  } while(pos != nullptr && ++pos);
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
  void* data = posix::malloc(MAX_MOUNT_MEM); // allocate memory
  if(data == NULL)
    return false;
  bool result = mount_bsd(device, path, filesystem, options, data);
  posix::free(data);
  return result;
}

bool mount_bsd(const char* device,
               const char* path,
               const char* filesystem,
               const char* options,
                     void* data) noexcept
#endif
{
  char fslist[1024] = { 0 };
  posix::strncpy(fslist, filesystem, 1024);
  const char *pos, *next;
  StringToken tok;
  int mountflags = 0;
  std::string optionlist;

  if(options != nullptr)
  {
    tok.setOrigin(options);
    pos = tok.pos();
    next = tok.next(',');
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
          if(next != nullptr)
            optionlist.append(1, ',');
          break;
      }
      pos = next;
      next = tok.next(',');
    } while(pos != nullptr && ++pos);
  }

  //MNT_UPDATE, MNT_RELOAD, and MNT_GETARGS


  tok.setOrigin(fslist);
  pos = tok.pos();
  next = tok.next(',');
  do
  {
    if(next != nullptr)
      *const_cast<char*>(next) = '\0';
#if !defined(BSD) && defined(__linux__)
    typedef unsigned long mnt_flag_t; // defined for mount()
    if(::mount(device, path, pos, mnt_flag_t(mountflags), optionlist.c_str()) == posix::success_response)
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
    switch(hash(pos))
    {
      case "iso9660"_hash:
      PARSE_ARG_CASE(cd9660);

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
/*
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
*/
    }
    if(parse_ok)
    {
# if defined(__NetBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(5,0,0)
      if(::mount(pos, path, mountflags, data, MAX_MOUNT_MEM) == posix::success_response)
        return true;

# elif (defined(__FreeBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(3,0,0)) || \
       (defined(__NetBSD__)  && KERNEL_VERSION_CODE >= KERNEL_VERSION(1,2,0)) || \
        defined(__OpenBSD__)    || \
        defined(__DragonFly__)  || \
        defined(__darwin__)     || \
        defined(__sunos__)
#  if defined(__sunos__)
      typedef caddr_t mount_data_t;
#  else
      typedef void* mount_data_t;
#  endif
      if(::mount(pos, path, mountflags, reinterpret_cast<mount_data_t>(data)) == posix::success_response)
        return true;

# else // ye olde "int type" based mount()
#  define FSTYPE_CASE(name, value) \
    case compiletime_hash(CASE_QUOTE(name), sizeof(CASE_QUOTE(name)) - 1) : \
      fstype = value; \
      break

      typedef caddr_t mount_data_t;
      int fstype = 0;
      switch(hash(pos))
      {
        // modern aliases
        FSTYPE_CASE(iso9660 , MOUNT_CD9660); // ISO9660 (aka CDROM) Filesystem
        FSTYPE_CASE(ffs     , MOUNT_UFS   ); // Fast Filesystem
        FSTYPE_CASE(msdosfs , MOUNT_MSDOS ); // MS/DOS Filesystem

        // filesystems
        FSTYPE_CASE(ufs   , MOUNT_UFS   ); // Fast Filesystem
        FSTYPE_CASE(nfs   , MOUNT_NFS   ); // Sun-compatible Network Filesystem
        FSTYPE_CASE(mfs   , MOUNT_MFS   ); // Memory-based Filesystem
        FSTYPE_CASE(msdos , MOUNT_MSDOS ); // MS/DOS Filesystem
        FSTYPE_CASE(lfs   , MOUNT_LFS   ); // Log-based Filesystem
        FSTYPE_CASE(lofs  , MOUNT_LOFS  ); // Loopback Filesystem
        FSTYPE_CASE(fdesc , MOUNT_FDESC ); // File Descriptor Filesystem
        FSTYPE_CASE(portal, MOUNT_PORTAL); // Portal Filesystem
        FSTYPE_CASE(null  , MOUNT_NULL  ); // Minimal Filesystem Layer
        FSTYPE_CASE(umap  , MOUNT_UMAP  ); // User/Group Identifier Remapping Filesystem
        FSTYPE_CASE(kernfs, MOUNT_KERNFS); // Kernel Information Filesystem
        FSTYPE_CASE(procfs, MOUNT_PROCFS); // /proc Filesystem
        FSTYPE_CASE(afs   , MOUNT_AFS   ); // Andrew Filesystem
        FSTYPE_CASE(cd9660, MOUNT_CD9660); // ISO9660 (aka CDROM) Filesystem
        FSTYPE_CASE(union , MOUNT_UNION ); // Union (translucent) Filesystem
        FSTYPE_CASE(devfs , MOUNT_DEVFS ); // existing device Filesystem
        FSTYPE_CASE(ext2fs, MOUNT_EXT2FS); // Linux EXT2FS
        FSTYPE_CASE(tfs   , MOUNT_TFS   ); // Netcon Novell filesystem
        FSTYPE_CASE(cfs   , MOUNT_CFS   ); // Coda filesystem
      }

      if(::mount(fstype, path, mountflags, reinterpret_cast<mount_data_t>(data)) == posix::success_response)
        return true;
# endif
    }
#else
# pragma message("No mount() implemented for this platform.  Please submit a patch.")
    errno = EOPNOTSUPP;
    return false;
#endif
    pos = next;
    next = tok.next(',');
  } while(pos != NULL && ++pos);

  return false;
}

// ====== UNMOUNT ======

#if defined(__ultrix__)
# define target_translator      dir2device
# define simple_unmount(x)      umount(device_id(x))
dev_t device_id(const char* device) noexcept
{
  struct stat buf;
  if(stat(device, &buf) == posix::error_response)
    return 0;
  return S_ISBLK(buf.st_mode) ? buf.st_rdev : 0;
}
#elif defined(__hpux__)
# define target_translator      dir2device
# define simple_unmount(x)      umount(x)
#elif defined(__sunos__)
# define target_translator      dir2device
# define simple_unmount(x)      unmount(x)
#elif defined(__minix__)
# define target_translator      device2dir
# define simple_unmount(x)      umount(x)
#elif (defined(__linux__) && KERNEL_VERSION_CODE < KERNEL_VERSION(2,1,116)) || \
       defined(__irix__)
# define target_translator      link_resolve
# define simple_unmount(x)      umount(x)
#elif defined(__linux__)
# define target_translator      link_resolve
# define flagged_unmount(x, y)  umount2(x, y)
#elif defined(__solaris__)
# define target_translator      device2dir
# define flagged_unmount(x, y)  umount2(x, y)
#elif defined(__tru64__)
# define target_translator      device2dir
# define flagged_unmount(x, y)  umount(x, y)
#elif defined(BSD)
# define target_translator      device2dir
# define flagged_unmount(x, y)  unmount(x, y)
#endif

static inline bool link_resolve(const char* input, char* output) noexcept
{
  assert(input != output);
  if(readlink(input, output, PATH_MAX) > 0 || errno == EINVAL)
  {
    posix::strncpy(output, input, PATH_MAX);
    return true;
  }
  return false;
}

static inline bool device2dir(const char* input, char* output) noexcept
{
  struct stat buf;
  if(stat(input, &buf) == posix::success_response)
  {
    if(S_ISLNK(buf.st_mode))
    {
      return link_resolve(input, output) &&
               device2dir(output, output);
    }
    else if(S_ISDIR(buf.st_mode))
    {
      if(input != output)
        posix::strncpy(output, input, PATH_MAX);
      return true;
    }
    else if(S_ISBLK(buf.st_mode) || S_ISCHR(buf.st_mode))
    {
      std::list<fsentry_t> table;
      if(mount_table(table))
        for(const fsentry_t& entry : table)
          if(!posix::strcmp(entry.device, input))
          {
            posix::strncpy(output, entry.path, PATH_MAX);
            return true;
          }
    }
  }
  return false;
}

static inline bool dir2device(const char* input, char* output) noexcept
{
  struct stat buf;
  if(stat(input, &buf) == posix::success_response)
  {
    if(S_ISLNK(buf.st_mode))
    {
      return link_resolve(input, output) &&
               dir2device(output, output);
    }
    else if(S_ISBLK(buf.st_mode) || S_ISCHR(buf.st_mode))
    {
      if(input != output)
        posix::strncpy(output, input, PATH_MAX);
      return true;
    }
    else if(S_ISDIR(buf.st_mode))
    {
      std::list<fsentry_t> table;
      if(mount_table(table))
        for(const fsentry_t& entry : table)
          if(!posix::strcmp(entry.path, input))
          {
            posix::strncpy(output, entry.device, PATH_MAX);
            return true;
          }
    }
  }
  return false;
}

bool unmount(const char* target, const char* options) noexcept
{
  char translated_target[PATH_MAX] = { 0 };

#if defined(simple_unmount)
# pragma message("Warning: unmount() options are not supported.")
  return target_translator(target, translated_target) &&
         simple_unmount(translated_target) != posix::success_response;

#elif defined(flagged_unmount)
  int flags = 0;
  if(options != nullptr)
  {
    StringToken tok(options);
    const char* pos = tok.pos();
    const char* next = tok.next(',');
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
      next = tok.next(',');
    } while(pos != nullptr && ++pos);
  }

  return target_translator(target, translated_target) &&
         flagged_unmount(translated_target, flags) != posix::success_response;
#else
# pragma message("No unmount() implemented for this platform.  Please submit a patch.")
  errno = EOPNOTSUPP;
  return false;
#endif
}
