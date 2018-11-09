#ifndef MOUNTPOINT_HELPERS_H
#define MOUNTPOINT_HELPERS_H

extern char* procfs_path;
extern char* sysfs_path;
extern char* devfs_path;
extern char* scfs_path;

bool initialize_paths(void) noexcept;

#endif // MOUNTPOINT_HELPERS_H
