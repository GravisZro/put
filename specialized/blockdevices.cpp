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

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 0x00000400
#endif


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
  typedef bool (*detector_t)(uint8_t*, blockdevice_t*);
  void detect(void);
  bool detect_ext(uint8_t* data, blockdevice_t* dev);
  bool detect_NULL(uint8_t* data, blockdevice_t* dev);

  static std::list<blockdevice_t> devices;
  static std::list<detector_t> detectors = { detect_ext, detect_NULL };



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
        // try to get the raw size of the partition (might be larger than the usable size)
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
        auto detectpos = detectors.begin();
        while(detectpos != detectors.end() &&
              !(*detectpos)(superblock, &dev)) // run filesystem detection functions until you find one
          ++detectpos;

        printf("device: %s - label: %s - fs: %s - size: %lu\n", dev.path, dev.label, dev.fstype, dev.size);
      }
    } // for each device
  } // end detect()


// DETECTION HELPER constexprs!

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

  template<typename T> constexpr uint32_t getFlags(T addr, uint32_t flags) { return getLE32(addr) & flags; }
  template<typename T> constexpr bool flagsAreSet(T addr, uint32_t flags) { return getFlags(addr, flags) == flags; }
  template<typename T> constexpr bool flagsNotSet(T addr, uint32_t flags) { return !getFlags(addr, flags); }


// FILESYSTEM DETECTION FUNCTIONS BELOW!

  // https://github.com/torvalds/linux/blob/master/fs/ext2/ext2.h
  // https://github.com/torvalds/linux/blob/master/fs/ext4/ext4.h
  // https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#The_Super_Block
  bool detect_ext(uint8_t* data, blockdevice_t* dev)
  {
    constexpr uint16_t ext_magic_number = 0xEF53; // s_magic

    enum compat_flags : uint32_t // s_feature_compat flags
    {
      has_journal     = 0x00000004,
    };

    enum incompat_flags : uint32_t // s_feature_incompat flags
    {
      filetype        = 0x00000002,
      recover         = 0x00000004,
      journal_dev     = 0x00000008,
      meta_bg         = 0x00000010,
    };

    enum ro_compat_flags : uint32_t // s_feature_ro_compat flags
    {
      sparse_super    = 0x00000001,
      large_file      = 0x00000002,
      btree_dir       = 0x00000004,
    };

    enum misc_flags : uint32_t // s_flags flags
    {
      dev_filesystem  = 0x00000004,
    };

    enum composite_flags : uint32_t // composite flags
    {
      EXT2_RO_compat_flags = ro_compat_flags::sparse_super | ro_compat_flags::large_file | ro_compat_flags::btree_dir,
      EXT2_incompat_flags  = incompat_flags::filetype | incompat_flags::meta_bg,
      EXT3_incompat_flags  = incompat_flags::filetype | incompat_flags::meta_bg | incompat_flags::recover,
    };

    enum offsets : uintptr_t
    {
      block_count          = 0x0004,
      block_size           = 0x0018,
      magic_number         = 0x0038, // s_magic

      compat_flags         = 0x005C, // s_feature_compat
      incompat_flags       = 0x0060, // s_feature_incompat
      ro_compat_flags      = 0x0064, // s_feature_ro_compat

      uuid                 = 0x0068, // s_uuid[16]
      label                = 0x0078, // s_volume_name[16]
      misc_flags           = 0x0160, // s_flags
    };

    if(getLE16(data + offsets::magic_number) != ext_magic_number ) // test not Ext2/3/4/4dev or JDB
      return false;

    uint32_t blocksize  = BLOCK_SIZE << getLE32(data + offsets::block_size); // filesystem block size
    uint64_t blockcount = getLE32(data + offsets::block_count); // filesystem block count
    dev->size = blockcount * blocksize; // store partitions usable size

    if(flagsAreSet(data + offsets::incompat_flags  , incompat_flags::journal_dev))
      std::strcpy(dev->fstype, "jbd");

    if(flagsNotSet(data + offsets::incompat_flags  , incompat_flags::journal_dev) &&
       flagsAreSet(data + offsets::misc_flags      , misc_flags::dev_filesystem))
      std::strcpy(dev->fstype, "ext4dev");

    if(flagsNotSet(data + offsets::incompat_flags  , incompat_flags::journal_dev) &&
       (getFlags(data + offsets::ro_compat_flags   , ~EXT2_RO_compat_flags) ||
        getFlags(data + offsets::incompat_flags    , ~EXT3_incompat_flags)) &&
       flagsNotSet(data + offsets::misc_flags      , misc_flags::dev_filesystem))
      std::strcpy(dev->fstype, "ext4");

    if(flagsAreSet(data + offsets::compat_flags    , compat_flags::has_journal) &&
       flagsNotSet(data + offsets::ro_compat_flags , ~EXT2_RO_compat_flags) &&
       flagsNotSet(data + offsets::incompat_flags  , ~EXT3_incompat_flags))
      std::strcpy(dev->fstype, "ext3");

    if(flagsNotSet(data + offsets::compat_flags    , compat_flags::has_journal) &&
       flagsNotSet(data + offsets::ro_compat_flags , ~EXT2_RO_compat_flags) &&
       flagsNotSet(data + offsets::incompat_flags  , ~EXT2_incompat_flags))
      std::strcpy(dev->fstype, "ext2");

    if(!dev->fstype[0])
      return false;

    std::memcpy(dev->uuid, data + offsets::uuid, 16);
    std::strncpy(dev->label, (const char*)data + offsets::label, 16);
    return true;
  }

  bool detect_NULL(uint8_t* data, blockdevice_t* dev)
  {
    printf("NOT A RECOGNIZED FILESYSTEM!  ");
    return true;
  }


} // end namespace
