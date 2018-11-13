#ifndef FSTABLE_H
#define FSTABLE_H

#include <list>
#include <string>

struct fsentry_t
{
  char* device;
  char* path;
  char* filesystems;
  char* options;
  int dump_frequency;
  int pass;

  fsentry_t(void) noexcept;
  fsentry_t(const char* _device,
            const char* _path,
            const char* _filesystems,
            const char* _options      = "defaults",
            const int _dump_frequency = 0,
            const int _pass           = 0) noexcept;
  ~fsentry_t(void) noexcept;

  //bool operator < (const fsentry_t&) const { return false; }
  bool operator == (const fsentry_t& other) const;
};

bool filesystem_table(std::list<struct fsentry_t>& table) noexcept;
bool mount_table(std::list<struct fsentry_t>& table) noexcept;

#endif // FSTABLE_H
