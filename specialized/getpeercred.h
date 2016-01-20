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

int getpeercred(int sockfd, proccred_t& cred);

#endif // GETPEERID_H

