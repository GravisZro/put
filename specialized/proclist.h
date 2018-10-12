#ifndef PROCLIST_H
#define PROCLIST_H

// STL
#include <vector>

// PDTK
#include <cxxutils/posix_helpers.h>

posix::error_t proclist(std::vector<pid_t>& list) noexcept;

#endif // PROCLIST_H
