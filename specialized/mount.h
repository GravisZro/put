#ifndef MOUNT_H
#define MOUNT_H

bool mount(const char* device,
          const char* path,
          const char* filesystem,
          const char* options) noexcept;

bool unmount(const char* path) noexcept;

#endif // MOUNT_H
