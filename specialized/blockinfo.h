#ifndef DISKINFO_H
#define DISKINFO_H

#include <cxxutils/posix_helpers.h>

struct blockinfo_t
{
  uint32_t block_size;
  uint64_t block_count;
};

bool block_info(const char* device, blockinfo_t& info) noexcept;

bool block_info(posix::fd_t fd, blockinfo_t& info) noexcept;

#endif // DISKINFO_H
