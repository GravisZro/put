#ifndef MOUNTPOINT_HELPERS_H
#define MOUNTPOINT_HELPERS_H

extern char* procfs_path;
extern char* sysfs_path;
extern char* devfs_path;
extern char* scfs_path;

bool reinitialize_paths(void) noexcept; // automaticlly called at program start

#endif // MOUNTPOINT_HELPERS_H
