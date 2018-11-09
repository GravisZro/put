#ifndef PROCLIST_H
#define PROCLIST_H

// STL
#include <vector>

// POSIX
#include <sys/types.h>

bool proclist(std::vector<pid_t>& list) noexcept;

#endif // PROCLIST_H
