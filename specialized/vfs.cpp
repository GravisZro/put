#include "vfs.h"

#include <cxxutils/posix_helpers.h>

template<typename A, typename B> constexpr A& to (B& b) { return *reinterpret_cast<A*>(&b); }




static_assert((S_IFIFO | S_IFCHR | S_IFDIR | S_IFBLK | S_IFREG | S_IFLNK | S_IFSOCK) >> 12 == 0x0F, "unexpected filetype identifiers");

enum filetype_e : uint16_t
{
  type_fifo      = S_IFIFO  >> 12,
  type_chardev   = S_IFCHR  >> 12,
  type_dir       = S_IFDIR  >> 12,
  type_blockdev  = S_IFBLK  >> 12,
  type_file      = S_IFREG  >> 12,
  type_link      = S_IFLNK  >> 12,
  type_socket    = S_IFSOCK >> 12,

  type_mask      = 0xF,
};

struct permissions_t
{
 uint16_t other_execute  : 1;
 uint16_t other_write    : 1;
 uint16_t other_read     : 1;

 uint16_t group_execute  : 1;
 uint16_t group_write    : 1;
 uint16_t group_read     : 1;

 uint16_t user_execute   : 1;
 uint16_t user_write     : 1;
 uint16_t user_read      : 1;

 uint16_t sticky_bit     : 1;
 uint16_t gid_bit        : 1;
 uint16_t uid_bit        : 1;

 filetype_e type         : 4;

 inline void setRead   (const bool value = true) { other_read    = group_read    = user_read    = (value ? 1 : 0); }
 inline void setWrite  (const bool value = true) { other_write   = group_write   = user_write   = (value ? 1 : 0); }
 inline void setExecute(const bool value = true) { other_execute = group_execute = user_execute = (value ? 1 : 0); }

 permissions_t(void) { to<uint16_t>(*this) = 0; } // start zeroed out
 permissions_t(const bool r,
               const bool w,
               const bool x,
               filetype_e t)
   : other_execute(x),
     other_write  (w),
     other_read   (r),
     group_execute(x),
     group_write  (w),
     group_read   (r),
     user_execute (x),
     user_write   (w),
     user_read    (r),
     sticky_bit   (0),
     gid_bit      (0),
     uid_bit      (0),
     type         (t) { }
};

static_assert(sizeof(permissions_t) == sizeof(uint16_t), "bad size: struct permissions_t");

// arbitrary assignment function
template<class A, class B>
constexpr void assign(A& a, B& b) { a = reinterpret_cast<A>(b); }

VFS::VFS()
{
  getdir = nullptr;
  utime  = nullptr;

  assign( getattr    , GetAttr     );
/*
  assign( readlink   , ReadLink    );
  assign( mknod      , MkNod       );
  assign( mkdir      , MkDir       );
  assign( unlink     , Unlink      );
  assign( rmdir      , RmDir       );
  assign( symlink    , SymLink     );
  assign( rename     , Rename      );
  assign( link       , Link        );
*/
  //assign( chmod      , Chmod       );
  //assign( chown      , Chown       );
  assign( truncate   , Truncate    );

  //assign( open       , Open        );

  assign( read       , Read        );
  assign( write      , Write       );
/*
  assign( statfs     , StatFS      );
  assign( flush      , Flush       );
  assign( release    , Release     );
  assign( fsync      , Fsync       );
  assign( setxattr   , SetXAttr    );
*/
  assign( getxattr   , GetXAttr    );
  assign( listxattr  , ListXAttr   );
/*
  assign( removexattr, RemoveXAttr );
  assign( opendir    , OpenDir     );
*/
  assign( readdir    , ReadDir     );
/*
  assign( releasedir , ReleaseDir  );
  assign( fsyncdir   , FsyncDir    );
  assign( init       , Init        );
  assign( access     , Access      );
  assign( create     , Create      );
  assign( ftruncate  , FTruncate   );
  assign( fgetattr   , FGetAttr    );
  assign( lock       , Lock        );
  assign( utimens    , UTimeNS     );
  assign( bmap       , BMap        );
  assign( ioctl      , IoCtl       );
  assign( poll       , Poll        );
  assign( write_buf  , WriteBuf    );
  assign( read_buf   , ReadBuf     );
  assign( flock      , FLock       );
  assign( fallocate  , FAllocate   );
*/
}

int VFS::GetXAttr(const char* path, const char* name, char* value, size_t size)
{
  if(std::string(name) == "exec")
  {
    strcpy(value, "/usr/bin/X;/bin/less");
    return (int)sizeof("/usr/bin/X;/bin/less");
  }
  return posix::success;
}

int VFS::ListXAttr(const char* path, char* list, size_t size)
{
  strcpy(list, "system.posix_acl_access\0");
  return 1;
}

int VFS::GetAttr(const char* path,
                 struct stat* statbuf)
{

  return posix::success;
}

int VFS::ReadDir(const char* path,
                           void* buf,
                           fuse_fill_dir_t filler,
                           off_t offset,
                           struct fuse_file_info* fileInfo)
{
  filler(buf, "." , nullptr, 0);
  filler(buf, "..", nullptr, 0);

  return posix::success;
}



int VFS::Read(const char* path,
                        char* buf,
                        size_t size,
                        off_t offset,
                        struct fuse_file_info* fileInfo)
{
  std::ostringstream stream;
  stream.rdbuf()->pubsetbuf(buf, size);
  stream << fs_pos->mixer_element->string();
  size = stream.str().size();
  return size;
}

int VFS::Write(const char* path,
                         const char* buf,
                         size_t size,
                         off_t offset,
                         struct fuse_file_info* fileInfo)
{
  std::stringstream stream;

  stream << buf; // buffer input
  stream >> value; // extract numeric value

  return size;
}

int VFS::Truncate(const char* path, off_t newSize)
{
  return posix::success;
}
