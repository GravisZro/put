// POSIX
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>
#include <specialized/getpeercred.h>

#if defined(LOCAL_PEERID)
// NetBSD
int getpeercred(int sockfd, proccred_t& cred)
{
  struct unpcbid data;
  socklen_t len = sizeof(data);

  int rval = getsockopt(s, 0, LOCAL_PEEREID, &data, &len);

  if(rval == posix::success)
  {
    cred.uid = data.unp_euid;
    cred.gid = data.unp_egid;
    cred.pid = data.unp_pid;
  }
  return rval;
}
#else
#include "../darwin/getpeercred.cpp"
#endif
