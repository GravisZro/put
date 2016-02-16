
// POSIX
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>
#include <specialized/getpeercred.h>

int getpeercred(int sockfd, proccred_t& cred)
{
  struct ucred data;
  socklen_t len = sizeof(data);

  int rval = getsockopt(sockfd, SOL_SOCKET, SO_PEERCRED, &data, &len);

  if(len != sizeof(data))
    rval = posix::error(std::errc::invalid_argument);

  if(rval == posix::success_response)
  {
    cred.pid = data.pid;
    cred.uid = data.uid;
    cred.gid = data.gid;
  }
  return rval;
}
