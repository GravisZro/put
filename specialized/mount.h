#ifndef MOUNT_H
#define MOUNT_H

int mount(const char* device,
          const char* path,
          const char* filesystem,
          const char* options) noexcept;

int unmount(const char* path) noexcept;

#endif // MOUNT_H
