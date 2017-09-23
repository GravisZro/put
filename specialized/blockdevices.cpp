#include "blockdevices.h"

#include <list>
#include <cxxutils/posix_helpers.h>

#include <unistd.h>

#if defined(__linux__)
#include <linux/fs.h>
#else // *BSD/Darwin
#include <sys/disklabel.h>
#include <sys/disk.h>
#endif

#include <arpa/inet.h>

#ifndef DEVFS_PATH
#define DEVFS_PATH          "/dev"
#endif

#define EXT_MAGIC_NUMBER                  0xEF53
#define EXT_COMPAT_FLAG_HAS_JOURNAL       0x00000004
#define EXT_INCOMPAT_FLAG_JOURNAL_DEV     0x00000008

/* for s_flags */
#define EXT_FLAGS_TEST_FILESYS              0x0004

/* for s_feature_compat */
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL     0x0004

/* for s_feature_ro_compat */
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE    0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM     0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK    0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE  0x0040

/* for s_feature_incompat */
#define EXT2_FEATURE_INCOMPAT_FILETYPE      0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER       0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV   0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG       0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS       0x0040 // extents support
#define EXT4_FEATURE_INCOMPAT_64BIT         0x0080
#define EXT4_FEATURE_INCOMPAT_MMP           0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG       0x0200

#define EXT2_FEATURE_RO_COMPAT_SUPP           (EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER | EXT2_FEATURE_RO_COMPAT_LARGE_FILE | EXT2_FEATURE_RO_COMPAT_BTREE_DIR)
#define EXT2_FEATURE_INCOMPAT_SUPP            (EXT2_FEATURE_INCOMPAT_FILETYPE | EXT2_FEATURE_INCOMPAT_META_BG)
#define EXT2_FEATURE_INCOMPAT_UNSUPPORTED     ~EXT2_FEATURE_INCOMPAT_SUPP
#define EXT2_FEATURE_RO_COMPAT_UNSUPPORTED    ~EXT2_FEATURE_RO_COMPAT_SUPP

#define EXT3_FEATURE_RO_COMPAT_SUPP           (EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER | EXT2_FEATURE_RO_COMPAT_LARGE_FILE | EXT2_FEATURE_RO_COMPAT_BTREE_DIR)
#define EXT3_FEATURE_INCOMPAT_SUPP            (EXT2_FEATURE_INCOMPAT_FILETYPE | EXT3_FEATURE_INCOMPAT_RECOVER | EXT2_FEATURE_INCOMPAT_META_BG)
#define EXT3_FEATURE_INCOMPAT_UNSUPPORTED     ~EXT3_FEATURE_INCOMPAT_SUPP
#define EXT3_FEATURE_RO_COMPAT_UNSUPPORTED    ~EXT3_FEATURE_RO_COMPAT_SUPP


#define EXT_BLOCK_COUNT_OFFSET            (superblock + 0x0004)
#define EXT_BLOCK_SIZE_OFFSET             (superblock + 0x0018)
#define EXT_MAGIC_NUMBER_OFFSET           (superblock + 0x0038)

#define EXT_COMPAT_FLAGS_OFFSET           (superblock + 0x005C) // s_feature_compat
#define EXT_INCOMPAT_FLAGS_OFFSET         (superblock + 0x0060) // s_feature_incompat
#define EXT_RO_COMPAT_FLAGS_OFFSET        (superblock + 0x0064) // s_feature_ro_compat

#define EXT_UUID_OFFSET                   (superblock + 0x0068) // s_uuid[16]
#define EXT_LABEL_OFFSET                  (superblock + 0x0078) // s_volume_name[16]
#define EXT_MISC_FLAGS_OFFSET             (superblock + 0x0160) // s_flags

uint64_t read(posix::fd_t fd, off_t offset, uint8_t* buffer, uint64_t length)
{
  if(::lseek(fd, offset, SEEK_SET) != offset)
    return 0;

  uint64_t remaining = length;
  ssize_t rval;
  for(uint8_t* pos = buffer; remaining > 0; pos += rval, remaining -= rval)
    rval = posix::read(fd, pos, remaining);

  return length - remaining;
}

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 0x400
#endif

struct uintle16_t
{
  uint8_t low;
  uint8_t high;
  constexpr operator uint16_t(void) const { return low + (high << 8); }
};
static_assert(sizeof(uintle16_t) == sizeof(uint16_t), "naughty compiler!");


struct uintle32_t
{
  uint8_t bottom;
  uint8_t low;
  uint8_t high;
  uint8_t top;
  constexpr operator uint32_t(void) const
  {
    return bottom +
        (low  <<  8) +
        (high << 16) +
        (top  << 24);
  }
};
static_assert(sizeof(uintle32_t) == sizeof(uint32_t), "naughty compiler!");

template<typename T> constexpr uint16_t get16(T* x) { return *reinterpret_cast<uint16_t*>(x); }
template<typename T> constexpr uint32_t get32(T* x) { return *reinterpret_cast<uint32_t*>(x); }
template<typename T> constexpr uint16_t getLE16(T* x) { return *reinterpret_cast<uintle16_t*>(x); }
template<typename T> constexpr uint32_t getLE32(T* x) { return *reinterpret_cast<uintle32_t*>(x); }

template<typename T> constexpr uint32_t flagsSet(T addr, uint32_t flags) { return getLE32(addr) & flags; }
template<typename T> constexpr bool flagsAreSet(T addr, uint32_t flags) { return flagsSet(addr, flags) == flags; }
template<typename T> constexpr bool flagsNotSet(T addr, uint32_t flags) { return !flagsSet(addr, flags); }


constexpr char uuid_digit(uint8_t* data, uint8_t digit)
  { return  "0123456789ABCDEF"[(digit & 1) ? (data[digit/2] & 0x0F) : (data[digit/2] >> 4)]; }

static bool uuid_matches(const char* str, uint8_t* data)
{
  size_t length = std::strlen(str);
  for(uint8_t digit = 0; digit < 32; ++digit, ++str)
  {
    if(!std::isxdigit(*str))
      { --length; ++str; }
    if(digit >= length || std::toupper(*str) != uuid_digit(data, digit))
      return false;
  }
  return true;
}

/*
static void uuid_decode(uint8_t* data, std::string& uuid)
{
  for(uint8_t digit = 0; digit < 32; ++digit)
    uuid.push_back(uuid_digit(data, digit));
}
*/

namespace blockdevices
{
  static std::list<blockdevice_t> devices;
  void detect(void);

#if defined(WANT_PROCFS)
  void init(const char* procfs_path)
  {
    devices.clear();
    char filename[PATH_MAX] = { 0 };
    std::strcpy(filename, procfs_path);
    std::strcat(filename, "/partitions");

    std::FILE* file = posix::fopen(filename, "r");
    if(file == nullptr)
      return;

    posix::ssize_t count = 0;
    posix::size_t size = 0;
    char* line = nullptr;
    char* begin = nullptr;
    while((count = ::getline(&line, &size, file)) != posix::error_response)
    {
      char* pos = line;
      if(!std::isspace(*pos)) // if line doesn't start with a space then it's not an entry!
        continue;

      while(*pos && std::isspace(*pos))
        ++pos;
      while(*pos && std::isdigit(*pos))
        ++pos;

      while(*pos && std::isspace(*pos))
        ++pos;
      while(*pos && std::isdigit(*pos))
        ++pos;

      while(*pos && std::isspace(*pos))
        ++pos;
      while(*pos && std::isdigit(*pos))
        ++pos;

      while(*pos && std::isspace(*pos))
        ++pos;

      if(!*pos) // if at end of line, skip
        continue;

      // read device name
      blockdevice_t dev;
      std::strcpy(dev.path, DEVFS_PATH);
      std::strcat(dev.path, "/");
      for(char* field = dev.path + std::strlen(dev.path);
          *pos && pos < dev.path + sizeof(blockdevice_t::path) && std::isgraph(*pos);
          ++pos, ++field)
        *field = *pos;
      {
        devices.emplace_back(dev);
      }
    }
    ::free(line); // use C free() because we're using C getline()
    line = nullptr;

    posix::fclose(file);

    detect();
  }

#else
  void init(void)
  {
#pragma message("Code needed to detect GEOM devices like gpart@")
    detect();
  }
#endif

  blockdevice_t* lookupByPath(const char* path) noexcept // finds device based on absolute path
  {
    for(blockdevice_t& dev : devices)
      if(!std::strcmp(path, dev.path))
        return &dev;
    return nullptr;
  }

  blockdevice_t* lookupByUUID(const char* uuid) noexcept // finds device based on uuid
  {
    for(blockdevice_t& dev : devices)
      if(uuid_matches(uuid, dev.uuid))
        return &dev;
    return nullptr;
  }

  blockdevice_t* lookupByLabel(const char* label) noexcept // finds device based on label
  {
    for(blockdevice_t& dev : devices)
      if(!std::strcmp(label, dev.label))
        return &dev;
    return nullptr;
  }

  blockdevice_t* lookup(const char* id)
  {
    for(blockdevice_t& dev : devices)
    {
      if(!std::strcmp(id, dev.label) ||
         !std::strcmp(id, dev.path) ||
         uuid_matches(id, dev.uuid))
        return &dev;
    }
    return nullptr;
  }

  void detect(void)
  {
    uint8_t superblock[BLOCK_SIZE];
    uint32_t blocksize;
    uint64_t blockcount;

    for(blockdevice_t& dev : devices)
    {
      posix::fd_t fd = posix::open(dev.path, O_RDONLY);
      dev.size = 0;

      if(fd != posix::error_response)
      {
  #if defined(BLKGETSIZE64) // Linux 2.6+
        posix::ioctl(fd, BLKGETSIZE64, &dev.size);

  #elif defined(DKIOCGETBLOCKCOUNT) // Darwin
        blocksize = 0;
        blockcount = 0;
        if(posix::ioctl(fd, DKIOCGETBLOCKSIZE , &blocksize ) > posix::error_response &&
           posix::ioctl(fd, DKIOCGETBLOCKCOUNT, &blockcount) > posix::error_response)
          dev.size = blockcount * blocksize;

  #elif defined(DIOCGMEDIASIZE) // current BSD
        posix::ioctl(fd, DIOCGMEDIASIZE, &dev.size);

  #elif defined(DIOCGDINFO) // old BSD
        struct disklabel info;
        if(posix::ioctl(fd, DIOCGDINFO, &info) > posix::error_response)
          dev.size = info.d_ncylinders * info.d_secpercyl * info.d_secsize;

  #else
        dev.size = ::lseek(fd, 0, SEEK_END); // behavior not defined in POSIX for devices but try as a last resort

  #pragma message("No device interface defined for this operating system.  Please add one to device.cpp!")
  #endif
      }

      std::memset(superblock, 0, BLOCK_SIZE);
      if(read(fd, BLOCK_SIZE, superblock, BLOCK_SIZE) == BLOCK_SIZE) // if read filesystem superblock
      {
        // see "struct ext2_super_block" in https://github.com/torvalds/linux/blob/master/fs/ext2/ext2.h
        // see "struct ext4_super_block" in https://github.com/torvalds/linux/blob/master/fs/ext4/ext4.h
        // https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#The_Super_Block
        if(getLE16(EXT_MAGIC_NUMBER_OFFSET) == EXT_MAGIC_NUMBER) // test if Ext2/3/4
        {
          blockcount = getLE32(EXT_BLOCK_COUNT_OFFSET);
          blocksize  = BLOCK_SIZE << getLE32(EXT_BLOCK_SIZE_OFFSET);

          if(flagsAreSet(EXT_INCOMPAT_FLAGS_OFFSET  , EXT3_FEATURE_INCOMPAT_JOURNAL_DEV))
            std::strcpy(dev.fstype, "jbd");

          if(flagsNotSet(EXT_INCOMPAT_FLAGS_OFFSET  , EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) &&
             flagsAreSet(EXT_MISC_FLAGS_OFFSET      , EXT_FLAGS_TEST_FILESYS))
            std::strcpy(dev.fstype, "ext4dev");

          if(flagsNotSet(EXT_INCOMPAT_FLAGS_OFFSET  , EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) &&
             (flagsSet(EXT_RO_COMPAT_FLAGS_OFFSET, EXT3_FEATURE_RO_COMPAT_UNSUPPORTED) ||
              flagsSet(EXT_INCOMPAT_FLAGS_OFFSET , EXT3_FEATURE_INCOMPAT_UNSUPPORTED)) &&
             flagsNotSet(EXT_MISC_FLAGS_OFFSET      , EXT_FLAGS_TEST_FILESYS))
            std::strcpy(dev.fstype, "ext4");

          if(flagsAreSet(EXT_COMPAT_FLAGS_OFFSET    , EXT3_FEATURE_COMPAT_HAS_JOURNAL) &&
             flagsNotSet(EXT_RO_COMPAT_FLAGS_OFFSET , EXT3_FEATURE_RO_COMPAT_UNSUPPORTED) &&
             flagsNotSet(EXT_INCOMPAT_FLAGS_OFFSET  , EXT3_FEATURE_INCOMPAT_UNSUPPORTED))
            std::strcpy(dev.fstype, "ext3");

          if(flagsNotSet(EXT_COMPAT_FLAGS_OFFSET    , EXT3_FEATURE_COMPAT_HAS_JOURNAL) &&
             flagsNotSet(EXT_RO_COMPAT_FLAGS_OFFSET , EXT2_FEATURE_RO_COMPAT_UNSUPPORTED) &&
             flagsNotSet(EXT_INCOMPAT_FLAGS_OFFSET  , EXT2_FEATURE_INCOMPAT_UNSUPPORTED))
            std::strcpy(dev.fstype, "ext2");

          std::memcpy(dev.uuid, EXT_UUID_OFFSET, 16);
          std::strncpy(dev.label, (const char*)EXT_LABEL_OFFSET, 16);
        }
        else if(true) // test other filesystem type
        {

        }


      }
    } // for each device
  } // end detect()
} // end namespace
