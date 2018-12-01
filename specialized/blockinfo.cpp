#include "blockinfo.h"

#if defined(__linux__) /* Linux */
#include <linux/fs.h>
#include <linux/hdreg.h> // ancient Linux
#elif defined(BSD) || defined(__darwin__) /* *BSD/Darwin */
#include <sys/disklabel.h>
#include <sys/disk.h>
#endif

bool block_info(const char* device, blockinfo_t& info) noexcept
{
  posix::fd_t fd = posix::open(device, O_RDONLY);

  if(fd == posix::error_response)
    return false;
  bool rvalue = block_info(fd, info);
  return posix::close(fd) && rvalue;
}

bool block_info(posix::fd_t fd, blockinfo_t& info) noexcept
{
  // get block size (if needed)
#if defined(BLKSSZGET) // Linux
  if(posix::ioctl(fd, BLKSSZGET, &info.block_size) <= posix::error_response)
    return false;
#elif defined(DKIOCGETBLOCKCOUNT) // Darwin
  if(posix::ioctl(fd, DKIOCGETBLOCKCOUNT, &info.block_size) <= posix::error_response)
    return false;
#elif defined(DIOCGSECTORSIZE) // BSD
  if(posix::ioctl(fd, DIOCGSECTORSIZE, &info.block_size) <= posix::error_response)
    return false;
#elif defined(BLOCK_SIZE) // Linux (hardcoded)
  info.block_size = BLOCK_SIZE;
#else
  info.block_size = 512; // default size
#endif

  // try to get the raw size of the partition (might be larger than the usable size)
#if defined(DKIOCGETBLOCKCOUNT) // Darwin
  if(posix::ioctl(fd, DKIOCGETBLOCKCOUNT, &info.block_count) <= posix::error_response)
    return false;

#elif defined(DIOCGMEDIASIZE) // BSD
  if(posix::ioctl(fd, DIOCGMEDIASIZE, &info.block_count) <= posix::error_response)
    return false;
  info.block_count /= info.block_size;

#elif defined(DIOCGDINFO) // old BSD
  struct disklabel info;
  if(posix::ioctl(fd, DIOCGDINFO, &info) <= posix::error_response)
    return false;
  info.block_size = info.d_secsize;
  info.block_count = info.d_ncylinders * info.d_secpercyl;

#elif defined(BLKGETSIZE64) // Linux 2.6+
  if(posix::ioctl(fd, BLKGETSIZE64, &info.block_count) <= posix::error_response)
    return false;
  info.block_count /= info.block_size;

#elif defined(BLKGETSIZE) // old Linux
  int bc = 0;
  if(posix::ioctl(fd, BLKGETSIZE, &bc) <= posix::error_response)
    return false;
  info.block_count = uint64_t(bc) * 512; // due to previously hardcoded sector size
  info.block_count /= info.block_size;

#elif defined(HDIO_GETGEO) // ancient Linux
  struct hd_geometry geom;
  if(posix::ioctl(fd, HDIO_GETGEO, &geom) <= posix::error_response)
    return false;
# error Sorry but the HDIO_GETGEO code is non-functional. ;(
  geom.heads; // number of heads
  geom.sectors; // number of sectors/track
  geom.cylinders; // number of cylinders, mod 65536
  geom.start; // starting sector of this partition.

#else
# pragma message("Falling back on lseek(fd, 0, SEEK_END) to get device size.  This is undefined behavior in POSIX.")
  info.block_count = uint64_t(::lseek(fd, 0, SEEK_END)); // behavior not defined in POSIX for devices but try as a last resort
  info.block_count /= info.block_size;
#endif
  return true;
}
