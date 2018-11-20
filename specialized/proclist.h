#ifndef PROCLIST_H
#define PROCLIST_H

// STL
#include <set>

// POSIX
#include <sys/types.h>

bool proclist(std::set<pid_t>& list) noexcept;

#endif // PROCLIST_H
