#include "peercred.h"

// PUT
#include <put/specialized/osdetect.h>
#include <put/cxxutils/socket_helpers.h>

#if defined(__solaris__) /* Solaris */
# include <ucred.h>

bool recv_cred(posix::fd_t socket, proccred_t& cred) noexcept
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
  return rval == posix::success_response;
}

bool send_cred(posix::fd_t) noexcept
{ return true; }

#elif defined(__darwin__)  /* Darwin */
# include <sys/ucred.h>

bool recv_cred(posix::fd_t socket, proccred_t& cred) noexcept
{
  xucred data;
  socklen_t len = sizeof(data);

  int rval = ::getsockopt(socket, SOL_LOCAL, LOCAL_PEERCRED, &data, &len);

  if(len != sizeof(data))
    rval = posix::error(posix::errc::invalid_argument);

  if(rval == posix::success_response)
  {
    cred.uid = data.cr_uid;
    cred.gid = data.cr_groups[0];

    len = sizeof(cred.pid);
    rval = ::getsockopt(socket, SOL_LOCAL, LOCAL_PEERPID, &cred.pid, &len);

    if(len != sizeof(cred.pid))
      rval = posix::error(posix::errc::invalid_argument);
  }
  return rval == posix::success_response;
}

bool send_cred(posix::fd_t) noexcept
{ return true; }

#elif defined (SO_PEERCRED) || defined (LOCAL_PEEREID) /* Linux/OpenBSD/legacy NetBSD */

# if defined (SO_PEERCRED) && \
    (defined(__linux__)    /* Linux    */ || \
     defined(__kfreebsd__) /* kFreeBSD */ )
constexpr int socket_cred_option = SO_PEERCRED;
typedef ucred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.uid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.gid; }

# elif defined(SO_PEERCRED) && defined(__OpenBSD__)
constexpr int socket_cred_option = SO_PEERCRED;
typedef sockpeercred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.uid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.gid; }

# elif defined(LOCAL_PEEREID) && defined(__NetBSD__) /* legacy NetBSD */
constexpr int socket_cred_option = LOCAL_PEEREID;
typedef unpcbid cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.unp_pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.unp_euid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.unp_egid; }

# elif defined(SO_PEERCRED)
#  error SO_PEERCRED macro detected but platform is unrecognized.  Please submit a patch!
# elif defined(LOCAL_PEEREID)
#  error LOCAL_PEEREID macro detected but platform is unrecognized.  Please submit a patch!
# endif

bool recv_cred(posix::fd_t socket, proccred_t& cred) noexcept
{
  cred_t data;
  socklen_t len = sizeof(data);

  int rval = ::getsockopt(socket, SOL_SOCKET, socket_cred_option, &data, &len);

  if(len != sizeof(data))
    rval = posix::error(posix::errc::invalid_argument);

  if(rval == posix::success_response)
  {
    cred.pid = peer_pid(data);
    cred.uid = peer_uid(data);
    cred.gid = peer_gid(data);
  }
  return rval == posix::success_response;
}

bool send_cred(posix::fd_t) noexcept
{ return true; }

#elif defined(SCM_CREDENTIALS) || defined(SCM_CREDS) || defined(LOCAL_CREDS)

# if defined(SCM_CREDENTIALS) /* Linux / kFreeBSD */
constexpr int credential_message = SCM_CREDENTIALS;
typedef ucred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.uid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.gid; }

# elif defined(SCM_CREDS) && defined(__NetBSD__) /* NetBSD */
constexpr int credential_message = SCM_CREDS;
typedef cred cred_t;
constexpr size_t cred_size = sizeof(cred_t);
constexpr pid_t peer_pid(const cred_t& data) { return data.sc_pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.sc_euid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.sc_egid; }

# elif defined(SCM_CREDS) && \
      (defined(__FreeBSD__)   /* FreeBSD      */ || \
       defined(__DragonFly__) /* DragonFlyBSD */ || \
       defined(__gnu_hurd__)  /* GNU/HURD     */ )
constexpr int credential_message = SCM_CREDS;
typedef cmsgcred cred_t;
constexpr size_t cred_size = sizeof(cred_t);
constexpr pid_t peer_pid(const cred_t& data) { return data.cmcred_pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.cmcred_euid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.cmcred_groups[0]; }

#elif defined(LOCAL_CRED) /* legacy FreeBSD / QNX */
constexpr int credential_message = LOCAL_CRED;
typedef sockcred cred_t;
constexpr size_t cred_size = SOCKCREDSIZE(16);
constexpr pid_t peer_pid(const cred_t&     ) { return -1; }
constexpr uid_t peer_uid(const cred_t& data) { return data.sc_euid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.sc_egid; }

# elif defined(SCM_CREDS)
#  error SCM_CREDS macro detected but platform is unrecognized.  Please submit a patch!
# endif

bool recv_cred(posix::fd_t socket, proccred_t& cred) noexcept
{
  struct msghdr message;
  union
  {
    char    rawbuffer[CMSG_SPACE(cred_size)];
    cmsghdr formatted;
  } cmsg_header;
  posix::memset(cmsg_header.rawbuffer, 0, sizeof(cmsg_header.rawbuffer)); // initialize memory

  char data = 0; // dummy data
  struct iovec iov = { &data, sizeof(data) };

  message.msg_iov = &iov;
  message.msg_iovlen = 1;
  message.msg_name = NULL;
  message.msg_namelen = 0;
  message.msg_control = &cmsg_header.formatted;
  message.msg_controllen = sizeof(cmsg_header.rawbuffer);
  message.msg_flags = 0;

  ssize_t nr = posix::recvmsg(socket, &message);
  if(nr == -1)
    return false;

  if(CMSG_FIRSTHDR(&message) == &cmsg_header.formatted &&
     cmsg_header.formatted.cmsg_len   <= sizeof(cmsg_header.rawbuffer) &&
     cmsg_header.formatted.cmsg_level == SOL_SOCKET &&
     cmsg_header.formatted.cmsg_type  == credential_message)
  {
    cred_t* data = reinterpret_cast<cred_t*>(CMSG_DATA(&cmsg_header.formatted));
    cred.pid = peer_pid(*data);
    cred.uid = peer_uid(*data);
    cred.gid = peer_gid(*data);
    return true;
  }
  return false;
}

bool send_cred(posix::fd_t socket) noexcept
{
  struct msghdr message;
  union
  {
    char    rawbuffer[CMSG_SPACE(cred_size)];
    cmsghdr formatted;
  } cmsg_header;
  posix::memset(cmsg_header.rawbuffer, 0, sizeof(cmsg_header.rawbuffer)); // initialize memory

  cmsg_header.formatted.cmsg_len   = CMSG_LEN(cred_size);
  cmsg_header.formatted.cmsg_level = SOL_SOCKET;
  cmsg_header.formatted.cmsg_type  = credential_message;

  char data = 0; // dummy data
  struct iovec iov = { &data, sizeof(data) };

  message.msg_iov = &iov;
  message.msg_iovlen = 1;
  message.msg_name = NULL;
  message.msg_namelen = 0;
  message.msg_control = &cmsg_header.formatted;
  message.msg_controllen = sizeof(cmsg_header.rawbuffer);
  message.msg_flags = 0;

  return posix::sendmsg(socket, &message) != -1;
}

#else

# pragma message("Socket credentials are not supported on this platform.")

bool recv_cred(posix::fd_t socket, proccred_t& cred) noexcept
{
  cred.pid = -1;
  cred.uid = -1;
  cred.gid = -1;
  errno = EOPNOTSUPP;
  return false;
}

int send_cred(posix::fd_t) noexcept
{
  errno = EOPNOTSUPP;
  return false;
}

#endif
