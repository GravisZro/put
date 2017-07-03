#ifndef PROCLIST_H
#define PROCLIST_H

// POSIX
#include <sys/types.h>

int proclist(pid_t* list, int max_length);

#endif // PROCLIST_H
