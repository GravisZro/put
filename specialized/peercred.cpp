#include "peercred.h"

// POSIX
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>

#if defined(__linux__) // Linux

int peercred(int socket, proccred_t& cred, int timeout) noexcept
{
  (void)timeout;
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

#include <ucred.h>

int peercred(int socket, proccred_t& cred, int timeout) noexcept
{
  (void)timeout;
  uproccred_t* data = nullptr;

  int rval = ::getpeerucred(socket, &data);

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

#elif defined(SCM_CREDS) // *BSD/Darwin/Hurd

#include <memory>

int peercred(posix::fd_t socket, creds_t& creds, int timeout) noexcept
{
  return posix::error_response;
  /*
  static const int enable = 1;
  ::setsockopt(socket, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable));

  msghdr header = {};
  iovec iov = { nullptr, 0 };
  std::unique_ptr<char> aux_buffer(new char[CMSG_SPACE(sizeof(creds_t))]);

  header.msg_iov = &iov;
  header.msg_iovlen = 1;
  header.msg_control = aux_buffer.get();
  header.msg_controllen = CMSG_SPACE(sizeof(creds_t));

  posix::ssize_t byte_count = posix::recvmsg(socket, &header, 0);

  if(!byte_count ||
     byte_count == posix::error_response)
    return false;

  cmsghdr* cmsg = CMSG_FIRSTHDR(&header);
  if(header.msg_controllen != CMSG_SPACE(sizeof(creds_t)))

      cmsg->cmsg_level == SOL_SOCKET &&
      cmsg->cmsg_type == SCM_CREDS &&
      cmsg->cmsg_len == CMSG_LEN(sizeof(creds_t));
      */
}

#elif defined(__unix__)

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif
