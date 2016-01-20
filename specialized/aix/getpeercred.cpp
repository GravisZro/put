// POSIX
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>
#include <specialized/getpeercred.h>

int getpeercred(int sockfd, proccred_t& cred)
{
  cred.pid = -1; // FIXME?
  return getpeereid(sockfd, &cred.uid, &cred.gid);
}
