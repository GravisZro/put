#ifndef MOUNT_H
#define MOUNT_H

bool mount(const char* device,
           const char* path,
           const char* filesystem,
           const char* options = nullptr) noexcept;

bool unmount(const char* target, const char* options = nullptr) noexcept;

#endif // MOUNT_H
