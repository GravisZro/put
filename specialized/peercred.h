#ifndef GETPEERID_H
#define GETPEERID_H

// PUT
#include <cxxutils/posix_helpers.h>

struct proccred_t
{
  pid_t pid;
  uid_t uid;
  gid_t gid;
};

bool recv_cred(posix::fd_t socket, proccred_t& cred) noexcept;
bool send_cred(posix::fd_t socket) noexcept;

#endif // GETPEERID_H

