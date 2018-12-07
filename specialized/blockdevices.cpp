#include "blockdevices.h"

// POSIX
#include <unistd.h>
#include <arpa/inet.h>

// STL
#include <list>

// PUT
#include <cxxutils/posix_helpers.h>
#include <specialized/osdetect.h>
#include <specialized/mountpoints.h>
#include <specialized/blockinfo.h>


uint64_t device_read(posix::fd_t fd, posix::off_t offset, uint8_t* buffer, uint64_t length)
{
  if(::lseek(fd, offset, SEEK_SET) != offset)
    return 0;

  uint64_t remaining = length;
  ssize_t rval;
  for(uint8_t* pos = buffer; remaining > 0; pos += rval, remaining -= uint64_t(rval))
    rval = posix::read(fd, pos, remaining);

  return length - remaining;
}

constexpr char uuid_digit(uint8_t* data, uint8_t digit)
  { return  "0123456789ABCDEF"[(digit & 1) ? (data[digit/2] & 0x0F) : (data[digit/2] >> 4)]; }

static bool uuid_matches(const char* str, uint8_t* data)
{
  size_t length = posix::strlen(str);
  for(uint8_t digit = 0; digit < 32; ++digit, ++str)
  {
    if(!::isxdigit(*str))
      { --length; ++str; }
    if(digit >= length || ::toupper(*str) != uuid_digit(data, digit))
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
  typedef bool (*detector_t)(blockinfo_t&, blockdevice_t&, uint8_t*);
  bool detect_filesystem(blockdevice_t& dev) noexcept;
  bool detect_ext(blockinfo_t& info, blockdevice_t& dev, uint8_t* data) noexcept;

  bool detect_NULL(blockinfo_t&, blockdevice_t&, uint8_t*) noexcept { return false; } // detection failed!

  static std::list<blockdevice_t> devices;
  static std::list<detector_t> detectors = { detect_ext, detect_NULL };

#if defined(__linux__)

  bool fill_device_list(void) noexcept
  {
    char filename[PATH_MAX] = { 0 };
    if(procfs_path == nullptr) // safety check
      return false;
    posix::snprintf(filename, PATH_MAX, "%s/partitions", procfs_path);

    posix::FILE* file = posix::fopen(filename, "r");
    if(file == NULL)
      return false;

    posix::ssize_t count = 0;
    posix::size_t size = 0;
    char* line = NULL; // getline will malloc
    while((count = posix::getline(&line, &size, file)) != posix::error_response)
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
      posix::strncpy(dev.path, devfs_path, sizeof(blockdevice_t::path) - sizeof("/"));
      posix::strcat(dev.path, "/");
      for(char* field = dev.path + posix::strlen(dev.path);
          *pos && pos < dev.path + sizeof(blockdevice_t::path) && std::isgraph(*pos);
          ++pos, ++field)
        *field = *pos;
      {
        devices.emplace_back(dev);
      }
    }
    ::free(line);
    line = nullptr;

    return posix::fclose(file);
  }
#elif (defined(__NetBSD__)  && KERNEL_VERION_CODE >= KERNEL_VERSION(1,6,0)) || \
      (defined(__OpenBSD__) && KERNEL_VERION_CODE >= KERNEL_VERSION(3,0,0))

#include <sys/sysctl.h>

  bool fill_device_list(void) noexcept
  {
    int mib[2] = { CTL_HW, HW_DISKNAMES };
    size_t len = 0;

    // Determine how much space to allocate.
    if(sysctl(mib, sizeof(mib), NULL, &len, NULL, 0) == posix::error_response)
      return false;

    char* p = (char *)::malloc(len);
    if(p == NULL)
      return false;

    // Populate the allocated area with the string returned by sysctl.
    if(sysctl(mib, 2, p, &len, NULL, 0) == posix::error_response)
      return false;


    return true;
  }

#elif defined(__FreeBSD__) && KERNEL_VERION_CODE >= KERNEL_VERSION(5,1,0)

  // STL
  #include <vector>

  // FreeBSD
  #include <geom/geom.h>
  #include <geom/geom_ctl.h>

  struct request_argument : gctl_req_arg
  {
    template<typename T>
    request_argument(const char* _name, T& _value)
    {
      name  = ::strdup(_name);
      nlen  = posix::strlen(name) + 1;
      value = &_value;
      flag  = GCTL_PARAM_RD;
      len   = sizeof(T);
    }

    request_argument(const char* _name, const char* _value)
    {
      name  = ::strdup(_name);
      nlen  = posix::strlen(name) + 1;
      value = _value;
      flag  = GCTL_PARAM_RD | GCTL_PARAM_ASCII;
      len   = posix::strlen(value) + 1;
    }

    ~request_argument(void) noexcept
    {
      if(valid_memory())
        ::free(name);
      name = NULL;
    }

    constexpr bool valid_memory(void) noexcept
      { return name != NULL; }
  };

  struct control_request : gctl_req
  {
    control_request(void)
    {
      version = GCTL_VERSION;
      lerror = BUFSIZ;		// XXX: arbitrary number
      error = calloc(1, lerror + 1);
    }
    ~control_request(void)
    {
      if(valid_memory())
        ::free(error);
      error = NULL;
    }

    constexpr bool valid_memory(void) noexcept
      { return error != NULL; }
  };

  bool fill_device_list(void) noexcept
  {
    control_request req;
    std::vector<request_argument> args;
    args.reserve(32);
    req.arg = args.data();

    args.emplace_back("class", "PART");
    args.emplace_back("verb", "show");
//    args.emplace_back("arg0", pp->lg_geom->lg_name);

    args.emplace_back("cmd", "list");

    char filename[PATH_MAX] = { 0 };
    if(devfs_path == nullptr) // safety check
      return false;
    posix::snprintf(filename, PATH_MAX, "%s/%s", devfs_path, PATH_GEOM_CTL);

    posix::fd_t fd = posix::open(filename, O_RDONLY);
    if(fd == posix::invalid_descriptor)
      return false;

    bool rval = posix::ioctl(fd, GEOM_CTL, &req) == posix::success_response;
    posix::close(fd);
    return rval;
  }
#else
# define NO_BLOCKDEV_SUPPORTED
# pragma message("No block device support implemented for this platform.  Please submit a patch.")
#endif

  bool init(void) noexcept
  {
#if defined(NO_BLOCKDEV_SUPPORTED)
    errno = EOPNOTSUPP;
    return false;
#else
    devices.clear();

    if(!fill_device_list())
      return false;

    bool rvalue = true;
    for(blockdevice_t& dev : devices)
      rvalue &= detect_filesystem(dev);

    return rvalue;
#endif
  }

  blockdevice_t* lookupByPath(const char* path) noexcept // finds device based on absolute path
  {
    for(blockdevice_t& dev : devices)
      if(!posix::strcmp(path, dev.path))
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
      if(!posix::strcmp(label, dev.label))
        return &dev;
    return nullptr;
  }

  blockdevice_t* lookup(const char* id) noexcept
  {
    for(blockdevice_t& dev : devices)
    {
      if(!posix::strcmp(id, dev.label) ||
         !posix::strcmp(id, dev.path) ||
         uuid_matches(id, dev.uuid))
        return &dev;
    }
    return nullptr;
  }

  blockdevice_t* probe(const char* path) noexcept
  {
    blockdevice_t dev;
    posix::strncpy(dev.path, path, sizeof(blockdevice_t::path));
    detect_filesystem(dev);
    if(!dev.fstype[0]) // if filesystem wasn't detected
      return nullptr;
    devices.emplace_back(dev);
    return &devices.back();
  }

  bool detect_filesystem(blockdevice_t& dev) noexcept
  {
    blockinfo_t info;
    posix::fd_t fd = posix::open(dev.path, O_RDONLY);

    if(fd == posix::error_response ||
       !block_info(fd, info))
      return false;

    uint8_t* superblock = static_cast<uint8_t*>(::malloc(info.block_size));
    if(superblock == NULL)
      return false;

    posix::memset(superblock, 0, info.block_size);
    if(device_read(fd, posix::off_t(info.block_size), superblock, info.block_size) != uint64_t(info.block_size)) // if read filesystem superblock
      return false;

    auto detectpos = detectors.begin();
    while(detectpos != detectors.end() &&
          !(*detectpos)(info, dev, superblock)) // run filesystem detection functions until you find one
      ++detectpos;

    posix::printf("device: %s - label: %s - fs: %s - block size: %u - block count: %lu\n", dev.path, dev.label, dev.fstype, dev.block_size, dev.block_count);
    ::free(superblock);

    return posix::close(fd);
  } // end detect()


// DETECTION HELPER constexprs!
# pragma pack(push, 1)
  struct uintle16_t
  {
    uint8_t low;
    uint8_t high;
    constexpr operator uint16_t(void) const noexcept { return uint16_t(low) | uint16_t(uint16_t(high) << 8); }
  };
# pragma pack(pop)
  static_assert(sizeof(uintle16_t) == sizeof(uint16_t), "naughty compiler!");

# pragma pack(push, 1)
  struct uintle32_t
  {
    uint8_t bottom;
    uint8_t low;
    uint8_t high;
    uint8_t top;
    constexpr operator uint32_t(void) const noexcept
    {
      return (uint32_t(bottom) <<  0) +
             (uint32_t(low   ) <<  8) +
             (uint32_t(high  ) << 16) +
             (uint32_t(top   ) << 24) ;
    }
  };
# pragma pack(pop)
  static_assert(sizeof(uintle32_t) == sizeof(uint32_t), "naughty compiler!");

  static inline uint16_t getBE16(uint8_t* x, uintptr_t offset) noexcept { return htons(*reinterpret_cast<uint16_t*>(x + offset)); }
  static inline uint32_t getBE32(uint8_t* x, uintptr_t offset) noexcept { return htonl(*reinterpret_cast<uint32_t*>(x + offset)); }
  static inline uint16_t getLE16(uint8_t* x, uintptr_t offset) noexcept { return ntohs(*reinterpret_cast<uint16_t*>(x + offset)); }
  static inline uint32_t getLE32(uint8_t* x, uintptr_t offset) noexcept { return ntohl(*reinterpret_cast<uint32_t*>(x + offset)); }

  static inline uint32_t getFlags(uint8_t* data, uintptr_t offset, uint32_t flags) noexcept { return getLE32(data, offset) & flags; }
  static inline bool allFlagsSet (uint8_t* data, uintptr_t offset, uint32_t flags) noexcept { return  getFlags(data, offset, flags) == flags; }
  static inline bool noFlagsSet  (uint8_t* data, uintptr_t offset, uint32_t flags) noexcept { return !getFlags(data, offset, flags); }
  static inline bool anyFlagsSet (uint8_t* data, uintptr_t offset, uint32_t flags) noexcept { return  getFlags(data, offset, flags); }


// FILESYSTEM DETECTION FUNCTIONS BELOW!

  // https://github.com/torvalds/linux/blob/master/fs/ext2/ext2.h
  // https://github.com/torvalds/linux/blob/master/fs/ext4/ext4.h
  // https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#The_Super_Block
  bool detect_ext(blockinfo_t& info, blockdevice_t& dev, uint8_t* data) noexcept
  {
    constexpr uint16_t ext_magic_number = 0xEF53; // s_magic

    enum e_compat_flags : uint32_t // s_feature_compat flags
    {
      has_journal     = 0x00000004,
    };

    enum e_incompat_flags : uint32_t // s_feature_incompat flags
    {
      filetype        = 0x00000002,
      recover         = 0x00000004,
      journal_dev     = 0x00000008,
      meta_bg         = 0x00000010,
    };

    enum e_ro_compat_flags : uint32_t // s_feature_ro_compat flags
    {
      sparse_super    = 0x00000001,
      large_file      = 0x00000002,
      btree_dir       = 0x00000004,
    };

    enum e_misc_flags : uint32_t // s_flags flags
    {
      dev_filesystem  = 0x00000004,
    };

    enum e_composite_flags : uint32_t // composite flags
    {
      EXT2_RO_compat_flags = e_ro_compat_flags::sparse_super | e_ro_compat_flags::large_file | e_ro_compat_flags::btree_dir,
      EXT2_incompat_flags  = e_incompat_flags::filetype | e_incompat_flags::meta_bg,
      EXT3_incompat_flags  = e_incompat_flags::filetype | e_incompat_flags::meta_bg | e_incompat_flags::recover,
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

    if(getLE16(data, offsets::magic_number) != ext_magic_number ) // test not Ext2/3/4/4dev or JDB
      return false;

    else if(allFlagsSet (data, offsets::incompat_flags , e_incompat_flags::journal_dev))
      posix::strncpy(dev.fstype, "jbd", sizeof(blockdevice_t::fstype));

    else if(noFlagsSet  (data, offsets::incompat_flags , e_incompat_flags::journal_dev) &&
            allFlagsSet (data, offsets::misc_flags     , e_misc_flags::dev_filesystem))
      posix::strncpy(dev.fstype, "ext4dev", sizeof(blockdevice_t::fstype));

    else if(noFlagsSet  (data, offsets::incompat_flags , e_incompat_flags::journal_dev) &&
            (allFlagsSet(data, offsets::ro_compat_flags, EXT2_RO_compat_flags) ||
             allFlagsSet(data, offsets::incompat_flags , EXT3_incompat_flags)) &&
            noFlagsSet  (data, offsets::misc_flags     , e_misc_flags::dev_filesystem))
      posix::strncpy(dev.fstype, "ext4", sizeof(blockdevice_t::fstype));

    else if(allFlagsSet (data, offsets::compat_flags   , e_compat_flags::has_journal) &&
            noFlagsSet  (data, offsets::ro_compat_flags, EXT2_RO_compat_flags) &&
            noFlagsSet  (data, offsets::incompat_flags , EXT3_incompat_flags))
      posix::strncpy(dev.fstype, "ext3", sizeof(blockdevice_t::fstype));

    else if(noFlagsSet  (data, offsets::compat_flags   , e_compat_flags::has_journal) &&
            noFlagsSet  (data, offsets::ro_compat_flags, EXT2_RO_compat_flags) &&
            noFlagsSet  (data, offsets::incompat_flags , EXT2_incompat_flags))
      posix::strncpy(dev.fstype, "ext2", sizeof(blockdevice_t::fstype));

    else
      return false;

    dev.block_size  = info.block_size << getLE32(data, offsets::block_size); // filesystem block size
    dev.block_count = getLE32(data, offsets::block_count); // filesystem block count

    posix::memcpy(dev.uuid, data + offsets::uuid, 16);
    posix::strncpy(dev.label, reinterpret_cast<char*>(data) + offsets::label, 16);
    return true;
  }

} // end namespace
