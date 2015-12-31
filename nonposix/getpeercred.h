#ifndef GETPEERID_H
#define GETPEERID_H

// POSIX
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>

struct proccred_t
{
  pid_t pid;
  uid_t uid;
  gid_t gid;
};

#if defined(SO_PEERCRED)
// Linux
static int getpeercred(int sockfd, proccred_t& cred)
{
  struct ucred data;
  socklen_t len = sizeof(data);

  int rval = getsockopt(sockfd, SOL_SOCKET, SO_PEERCRED, &data, &len);

  if(len != sizeof(data))
  {
    errno = EINVAL;
    rval = -1;
  }

  if(rval == posix::success)
  {
    cred.pid = data.pid;
    cred.uid = data.uid;
    cred.gid = data.gid;
  }
  return rval;
}
#elif defined(LOCAL_PEERCRED)
// Older FreeBSD
#include <sys/ucred.h>
static int getpeercred(int sockfd, proccred_t& cred)
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
    cred.pid = -1; // FIXME
    cred.uid = data.cr_uid;
    cred.gid = data.cr_gid;
  }
  return rval;
}
#elif defined(LOCAL_PEEREID)
// NetBSD
static int getpeercred(int sockfd, proccred_t& cred)
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

#elif defined(__sun) && defined(__SVR4)
// Solaris
#include <ucred.h>

static int getpeercred(int sockfd, proccred_t& cred)
{
  uproccred_t* data = nullptr;

  int rval = getpeerucred(sock, &data);

  if(rval == posix::success)
  {
    cred.pid = ucred_getpid (data);
    cred.uid = ucred_geteuid(data);
    cred.gid = ucred_getegid(data);
    ucred_free(data);
    if (cred.pid == (pid_t) (-1) ||
        cred.uid == (uid_t) (-1) ||
        cred.gid == (gid_t) (-1))
      rval = posix::error_response;
  }
  return rval;
}
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/param.h>

static int getpeercred(int sockfd, proccred_t& cred)
{
  cred.pid = -1; // FIXME
  return getpeereid(sockfd, &cred.uid, &cred.gid);
}
#else
#error no acceptible way to implement getpeercred
#endif


#endif // GETPEERID_H

