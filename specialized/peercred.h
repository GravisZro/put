#ifndef GETPEERID_H
#define GETPEERID_H

// POSIX
#include <sys/types.h>

struct proccred_t
{
  pid_t pid;
  uid_t uid;
  gid_t gid;
};

// NOTE: both client _and_ server must call this simutaneously (or it breaks _many_ platforms)
int peercred(int socket, proccred_t& cred) noexcept;

#endif // GETPEERID_H

