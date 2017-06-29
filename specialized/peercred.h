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

int peercred(int sockfd, proccred_t& cred) noexcept;

#endif // GETPEERID_H

