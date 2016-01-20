// POSIX
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>
#include <specialized/getpeercred.h>

#if defined(LOCAL_PEERCRED)
// Older FreeBSD
#include <sys/ucred.h>
int getpeercred(int sockfd, proccred_t& cred)
{
  struct xucred data;
  ACCEPT_TYPE_ARG3 len = sizeof(data);

  int rval = getsockopt(sock, 0, LOCAL_PEERCRED, &data, &so_len);

  if(len != sizeof(data) || data.cr_version != XUCRED_VERSION))
  {
    errno = EINVAL;
    rval = -1;
  }

  if(rval == posix::success)
  {
    cred.pid = -1; // FIXME?
    cred.uid = data.cr_uid;
    cred.gid = data.cr_gid;
  }
  return rval;
}
#else
#include "../darwin/getpeercred.cpp"
#endif
