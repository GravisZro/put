#ifndef BLOCKDEVICE_H
#define BLOCKDEVICE_H

#include <cxxutils/posix_helpers.h>

struct blockdevice_t
{
  char     path  [PATH_MAX];
  uint8_t  uuid  [16];
  char     label [256];
  char     fstype[256];
  uint32_t block_size;
  uint64_t block_count;
  bool     clean;

  blockdevice_t(void) noexcept { posix::memset(this, 0, sizeof(blockdevice_t)); }
  blockdevice_t(blockdevice_t& other) noexcept { posix::memcpy(this, &other, sizeof(blockdevice_t)); }
};

namespace blockdevices
{
  extern bool init(void) noexcept;
  extern blockdevice_t* probe(const char* path) noexcept; // probe a device based on absolute path

  extern blockdevice_t* lookup(const char* id) noexcept; // finds device based on absolute path, uuid or label
  extern blockdevice_t* lookupByPath(const char* path) noexcept; // finds device based on absolute path
  extern blockdevice_t* lookupByUUID(const char* uuid) noexcept; // finds device based on uuid
  extern blockdevice_t* lookupByLabel(const char* label) noexcept; // finds device based on label
}

#endif // BLOCKDEVICE_H
