#include "peercred.h"

// POSIX
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>

#if defined(SO_PEERCRED) // Linux
#pragma message("Information: using SO_PEERCRED code")

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

#elif defined(__sun) && defined(__SVR4) // Solaris
#pragma message("Information: using Solaris code")

#include <ucred.h>

int peercred(int socket, proccred_t& cred) noexcept
{
  uproccred_t* data = nullptr;

  int rval = ::getpeerucred(socket, &data);

  if(rval == posix::success_response)
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

#elif defined(LOCAL_PEEREID) // NetBSD
#pragma message("Information: using LOCAL_PEEREID code")

int peercred(int socket, proccred_t& cred) noexcept
{
  struct unpcbid data;
  socklen_t len = sizeof(data);

  int rval = getsockopt(socket, SOL_SOCKET, LOCAL_PEEREID, &data, &len);

  if(len != sizeof(data))
    rval = posix::error(std::errc::invalid_argument);

  if(rval == posix::success_response)
  {
    cred.pid = data.unp_pid;
    cred.uid = data.unp_euid;
    cred.gid = data.unp_egid;
  }
  return rval;
}

#elif defined(SCM_CREDS) // *BSD/Darwin/Hurd
#pragma message("Information: using SCM_CREDS code")

#if defined(LOCAL_CREDS) && !defined(SO_PASSCRED)
# define SO_PASSCRED  LOCAL_CREDS
#endif

#ifndef SOL_SOCKET
# define SOL_SOCKET   0
#endif

#if defined(__FreeBSD__)
typedef sockcred creds_t;
#elif defined(__NetBSD__)
typedef cred creds_t;
#endif

#if !defined(SOCKCREDSIZE)
#define SOCKCREDSIZE(x)   CMSG_SPACE(sizeof(creds_t) + (sizeof(gid_t) * x))
#endif

// size = SOCKCREDSIZE(((creds_t*)CMSG_DATA(cmptr))->sc_ngroups);

int peercred(int socket, proccred_t& cred) noexcept
{
  return posix::error_response;
  /*
  static const int enable = 1;

  ::setsockopt(socket, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable));


  msghdr header = {};
  iovec iov = { nullptr, 0 };
  char* aux_buffer = static_cast<char*>(::malloc(CMSG_SPACE(sizeof(creds_t))));

  header.msg_iov = &iov;
  header.msg_iovlen = 1;
  header.msg_control = aux_buffer;
  header.msg_controllen = CMSG_SPACE(sizeof(creds_t));

   cmsg_len = CMSG_LEN(SOCKCREDSIZE(ngroups))
                     cmsg_level = SOL_SOCKET
                     cmsg_type = SCM_CREDS

  posix::ssize_t byte_count = posix::recvmsg(socket, &header, 0);

  if(!byte_count ||
     byte_count == posix::error_response)
    return false;

  cmsghdr* cmsg = CMSG_FIRSTHDR(&header);
  if(header.msg_controllen != CMSG_SPACE(sizeof(creds_t)))

      cmsg->cmsg_level == SOL_SOCKET &&
      cmsg->cmsg_type == SCM_CREDS &&
      cmsg->cmsg_len == CMSG_LEN(sizeof(creds_t));

  ::free(aux_buffer);
      */
}

#elif defined(__unix__)

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif
