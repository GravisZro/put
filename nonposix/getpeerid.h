#ifndef GETPEERID_H
#define GETPEERID_H

// POSIX
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>

#if defined(_AIX)

#elif defined(SO_PEERCRED)
static int getpeereid(int sockfd, uid_t* uid, gid_t* gid)
{
  struct ucred cred;
  socklen_t len = sizeof(cred);

  int rval = getsockopt(sockfd, SOL_SOCKET, SO_PEERCRED, &cred, &len);
  if(rval == posix::success && len == sizeof(cred))
  {
    *uid = cred.uid;
    *gid = cred.gid;
  }
  return rval;
}
#elif defined(LOCAL_PEERCRED)

static int getpeereid(int sockfd, uid_t* uid, gid_t* gid)
{
  struct xucred cred;
  ACCEPT_TYPE_ARG3 len = sizeof(cred);

  int rval = getsockopt(sock, 0, LOCAL_PEERCRED, &cred, &so_len);
  if(rval == posix::success &&
     len == sizeof(cred) &&
     cred.cr_version == XUCRED_VERSION)
  {
    *uid = cred.cr_uid;
    *gid = cred.cr_gid;
  }
  return rval;
}

#elif defined(__sun) && defined(__SVR4)
#include <ucred.h>

static int getpeereid(int sockfd, uid_t* uid, gid_t* gid)
{
  ucred_t *cred = nullptr;

  int rval = getpeerucred(sock, &cred);
  if(rval == posix::success)
  {
    *uid = ucred_geteuid(cred);
    *gid = ucred_getegid(cred);
    ucred_free(cred);
    if (*uid == (uid_t) (-1) ||
        *gid == (gid_t) (-1))
      rval == posix::error_response;
  }
  return rval;
}
#else
#error no acceptible way to implement getpeereid
#endif


#endif // GETPEERID_H

