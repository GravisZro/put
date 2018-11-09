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

bool recv_cred(int socket, proccred_t& cred) noexcept;
bool send_cred(int socket) noexcept;

#endif // GETPEERID_H

