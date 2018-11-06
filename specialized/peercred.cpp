#include "peercred.h"

// POSIX
#include <sys/socket.h>
#include <sys/un.h>

// PUT
#include <specialized/osdetect.h>
#include <cxxutils/posix_helpers.h>

#if defined(__solaris__) // Solaris
#pragma message("Information: using Solaris code")

#include <ucred.h>

int recv_cred(int socket, proccred_t& cred) noexcept
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

int send_cred(int) noexcept
{ return posix::success_response; }

#elif defined (SO_PEERCRED) || defined (LOCAL_PEEREID) /* Linux/OpenBSD/Old NetBSD */

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

int recv_cred(int socket, proccred_t& cred) noexcept
{
  cred_t data;
  socklen_t len = sizeof(data);

  int rval = ::getsockopt(socket, SOL_SOCKET, socket_cred_option, &data, &len);

  if(len != sizeof(data))
    rval = posix::error(std::errc::invalid_argument);

  if(rval == posix::success_response)
  {
    cred.pid = peer_pid(data);
    cred.uid = peer_uid(data);
    cred.gid = peer_gid(data);
  }
  return rval;
}

int send_cred(int) noexcept
{ return posix::success_response; }

#elif defined(SCM_CREDENTIALS) || defined(SCM_CREDS) || defined(LOCAL_PEERCRED)

# if defined(SCM_CREDENTIALS) /* Linux / kFreeBSD */
constexpr int credential_message = SCM_CREDENTIALS;
typedef ucred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.uid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.gid; }

# elif defined(SCM_CREDS) && defined(__NetBSD__) /* NetBSD */
constexpr int credential_message = SCM_CREDS;
typedef cred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.sc_pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.sc_euid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.sc_egid; }

# elif defined(SCM_CREDS) && \
      (defined(__FreeBSD__)   /* FreeBSD      */ || \
       defined(__DragonFly__) /* DragonFlyBSD */ || \
       defined(__gnu_hurd__)  /* GNU/HURD     */ )
constexpr int credential_message = SCM_CREDS;
typedef cmsgcred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.cmcred_pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.cmcred_euid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.cmcred_groups[0]; }

# elif defined(LOCAL_PEERCRED)  /* Darwin / Old FreeBSD */
#include <sys/ucred.h>
constexpr int credential_message = LOCAL_PEERCRED;
typedef xucred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return -1; }
constexpr uid_t peer_uid(const cred_t& data) { return data.cr_uid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.cr_groups[0]; }

# elif defined(SCM_CREDS)
#  error SCM_CREDS macro detected but platform is unrecognized.  Please submit a patch!
# endif

int recv_cred(int socket, proccred_t& cred) noexcept
{
  struct msghdr message;
  union
  {
    char    rawbuffer[CMSG_SPACE(sizeof(cred_t))];
    cmsghdr formatted;
  } cmsg_header;

  static_assert(sizeof(cmsg_header.rawbuffer) == CMSG_SPACE(sizeof(cred_t)), "sizeof() is acting improperly");
  std::memset(cmsg_header.rawbuffer, 0, sizeof(cmsg_header.rawbuffer)); // initialize memory

  char data = 0; // dummy data
  struct iovec iov = { &data, sizeof(data) };

  message.msg_iov = &iov;
  message.msg_iovlen = 1;
  message.msg_name = NULL;
  message.msg_namelen = 0;
  message.msg_control = cmsg_header.formatted;
  message.msg_controllen = sizeof(cmsg_header.rawbuffer);
  message.msg_flags = 0;

  int nr = ::recvmsg(socket, &message, 0);
  if(nr == posix::error_response)
    return posix::error_response;

  if(CMSG_FIRSTHDR(&message) == &cmsg_header.formatted &&
     cmsg_header.formatted.cmsg_len   <= sizeof(cmsg_header.rawbuffer) &&
     cmsg_header.formatted.cmsg_level == SOL_SOCKET &&
     cmsg_header.formatted.cmsg_type  == credential_message)
  {
    const cred_t& data = *CMSG_DATA(&cmsg_header.formatted);
    cred.pid = peer_pid(data);
    cred.uid = peer_uid(data);
    cred.gid = peer_gid(data);
    return posix::success_response;
  }
  return posix::error_response;
}


int send_cred(int socket) noexcept
{
  struct msghdr message;
  union
  {
    char    rawbuffer[CMSG_SPACE(sizeof(cred_t))];
    cmsghdr formatted;
  } cmsg_header;

  static_assert(sizeof(cmsg_header.rawbuffer) == CMSG_SPACE(sizeof(cred_t)), "sizeof() is acting improperly");
  std::memset(cmsg_header.rawbuffer, 0, sizeof(cmsg_header.rawbuffer)); // initialize memory

  cmsg_header.formatted.cmsg_len   = CMSG_LEN(sizeof(cred_t));
  cmsg_header.formatted.cmsg_level = SOL_SOCKET;
  cmsg_header.formatted.cmsg_type  = credential_message;

  char data = 0; // dummy data
  struct iovec iov = { &data, sizeof(data) };

  message.msg_iov = &iov;
  message.msg_iovlen = 1;
  message.msg_name = NULL;
  message.msg_namelen = 0;
  message.msg_control = cmsg_header.formatted;
  message.msg_controllen = sizeof(cmsg_header.rawbuffer);
  message.msg_flags = 0;

  return ::sendmsg(socket, &message, 0);
}

#else

#pragma message("Socket credentials are not supported on the platform.")

int recv_cred(int socket, proccred_t& cred) noexcept
{
  cred.pid = -1;
  cred.uid = -1;
  cred.gid = -1;
  return posix::error(ENOSYS);
}

int send_cred(int) noexcept
{ return posix::error(ENOSYS); }

#endif
