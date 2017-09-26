#ifndef BLOCKDEVICE_H
#define BLOCKDEVICE_H

#include <cstdint>
#include <cstring>
#include <climits>

#if defined(__linux__)
#define WANT_PROCFS
#endif

struct blockdevice_t
{
  char     path  [PATH_MAX];
  uint8_t  uuid  [16];
  char     label [256];
  char     fstype[256];
  uint64_t size;
  bool     clean;

  blockdevice_t(void) { std::memset(this, 0, sizeof(blockdevice_t)); }
  blockdevice_t(blockdevice_t& other) { std::memcpy(this, &other, sizeof(blockdevice_t)); }
};

namespace blockdevices
{
#if defined(WANT_PROCFS)
  void init(const char* procfs_path) noexcept;
#else
  void init(void) noexcept;
#endif

  blockdevice_t* probe(const char* path) noexcept; // probe a device based on absolute path

  blockdevice_t* lookup(const char* id) noexcept; // finds device based on absolute path, uuid or label
  blockdevice_t* lookupByPath(const char* path) noexcept; // finds device based on absolute path
  blockdevice_t* lookupByUUID(const char* uuid) noexcept; // finds device based on uuid
  blockdevice_t* lookupByLabel(const char* label) noexcept; // finds device based on label
}

#endif // BLOCKDEVICE_H
