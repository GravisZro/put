#ifndef VFS_H
#define VFS_H

// POSIX
#include <sys/types.h>
#include <stdint.h>

// non-POSIX
#define FUSE_USE_VERSION 30
#include <fuse.h>


class VFS : public fuse_operations
{
public:
  VFS();

  static int GetAttr     (const char* path, struct stat* statbuf);
  static int ReadLink    (const char* path, char* link, posix::size_t size);
  static int MkNod       (const char* path, mode_t mode, dev_t dev);
  static int MkDir       (const char* path, mode_t mode);
  static int Unlink      (const char* path);
  static int RmDir       (const char* path);
  static int SymLink     (const char* path, const char* link);
  static int Rename      (const char* path, const char* newpath);
  static int Link        (const char* path, const char* newpath);
  static int Chmod       (const char* path, mode_t mode);
  static int Chown       (const char* path, uid_t uid, gid_t gid);
  static int Truncate    (const char* path, off_t newSize);
  static int Open        (const char* path, struct fuse_file_info* fileInfo);
  static int Read        (const char* path,       char* buf, posix::size_t size, off_t offset, struct fuse_file_info* fileInfo);
  static int Write       (const char* path, const char* buf, posix::size_t size, off_t offset, struct fuse_file_info* fileInfo);
  static int StatFS      (const char* path, struct statvfs* statInfo);
  static int Flush       (const char* path, struct fuse_file_info* fileInfo);
  static int Release     (const char* path, struct fuse_file_info* fileInfo);
  static int Fsync       (const char* path, int datasync, struct fuse_file_info* fi);
  static int SetXAttr    (const char* path, const char* name, const char* value, posix::size_t size, int flags);
  static int GetXAttr    (const char* path, const char* name, char* value, posix::size_t size);
  static int ListXAttr   (const char* path, char* list, posix::size_t size);
  static int RemoveXAttr (const char* path, const char* name);
  static int OpenDir     (const char* path, struct fuse_file_info* fileInfo);
  static int ReadDir     (const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fileInfo);
  static int ReleaseDir  (const char* path, struct fuse_file_info* fileInfo);
  static int FsyncDir    (const char* path, int datasync, struct fuse_file_info* fileInfo);
  static void* Init      (struct fuse_conn_info* conn);
  static int Access      (const char* path, int mask);
  static int Create      (const char* path, mode_t mode, struct fuse_file_info* fileInfo);
  static int FTruncate   (const char* path, off_t offset, struct fuse_file_info* fileInfo);
  static int FGetAttr    (const char* path, struct stat* statInfo, struct fuse_file_info* fileInfo);
  static int Lock        (const char* path, struct fuse_file_info* fileInfo, int cmd, struct flock* locks);
  static int UTimeNS     (const char* path, const struct timespec tv[2]);
  static int BMap        (const char* path, posix::size_t blocksize, uint64_t* idx);
  static int IoCtl       (const char* path, int cmd, void* arg, struct fuse_file_info* fileInfo, unsigned int flags, void* data);
  static int Poll        (const char* path, struct fuse_file_info* fileInfo, struct fuse_pollhandle* ph, unsigned* reventsp);
  static int WriteBuf    (const char* path, struct fuse_bufvec* buf, off_t offset, struct fuse_file_info* fileInfo);
  static int ReadBuf     (const char* path, struct fuse_bufvec** bufp, posix::size_t size, off_t offset, struct fuse_file_info* fileInfo);
  static int FLock       (const char* path, struct fuse_file_info* fileInfo, int operation);
  static int FAllocate   (const char* path, int mode, off_t offset, off_t length, struct fuse_file_info* fileInfo);
};

#endif // VFS_H
