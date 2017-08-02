#include "peercred.h"

// POSIX
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>

#if defined(__linux__) // Linux

int peercred(int socket, proccred_t& cred) noexcept
{
  struct ucred data;
  socklen_t len = sizeof(data);

  int rval = getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &data, &len);

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

#elif defined(LOCAL_PEERCRED) && !defined(__APPLE__) // Old FreeBSD
#include <sys/ucred.h>
int peercred(int socket, proccred_t& cred) noexcept
{
  struct xucred data;
  ACCEPT_TYPE_ARG3 len = sizeof(data);

  int rval = getsockopt(socket, 0, LOCAL_PEERCRED, &data, &so_len);

  if(len != sizeof(data) || data.cr_version != XUCRED_VERSION)
    rval = posix::error(std::errc::invalid_argument);

  if(rval == posix::success)
  {
    cred.pid = -1; // MUST be fixed
    cred.uid = data.cr_uid;
    cred.gid = data.cr_gid;
  }
  return rval;
}


#elif defined(BSD) || defined(__APPLE__) // *BSD or Darwin

// POSIX
#include <sys/types.h>

int peercred(int socket, proccred_t& cred) noexcept
{
  cred.pid = -1; // FIXME?
  return getpeereid(socket, &cred.uid, &cred.gid);
}

#elif defined(__sun) && defined(__SVR4) // Solaris
#include <ucred.h>

int peercred(int socket, proccred_t& cred) noexcept
{
  uproccred_t* data = nullptr;

  int rval = getpeerucred(socket, &data);

  if(rval == posix::success)
  {
    cred.pid = ucred_getpid (data);
    cred.uid = ucred_geteuid(data);
    cred.gid = ucred_getegid(data);
    ucred_free(data);
    if (cred.pid == pid_t(-1) ||
        cred.uid == uid_t(-1) ||
        cred.gid == gid_t(-1))
      rval = posix::error_response;
  }
  return rval;
}


#elif defined(__unix__)

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif
