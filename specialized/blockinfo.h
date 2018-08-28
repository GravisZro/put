#ifndef DISKINFO_H
#define DISKINFO_H

#include <cxxutils/posix_helpers.h>

struct block_info_t
{
  uint32_t block_size;
  uint64_t block_count;
};

bool block_info (const char* block_device, block_info_t& info) noexcept;

#endif // DISKINFO_H
