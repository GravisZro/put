// POSIX
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>
#include <specialized/getpeercred.h>

// Solaris
#include <ucred.h>

int getpeercred(int sockfd, proccred_t& cred)
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
