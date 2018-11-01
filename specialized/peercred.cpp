#include "peercred.h"

// POSIX
#include <sys/socket.h>
#include <sys/un.h>

// PDTK
#include <cxxutils/error_helpers.h>

#if defined(__sun) && defined(__SVR4) // Solaris
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

# if defined (SO_PEERCRED) && defined(__linux__)
constexpr int socket_cred_option = SO_PEERCRED;
typedef struct ucred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.uid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.gid; }

# elif defined (SO_PEERCRED) && defined(__OpenBSD__)
constexpr int socket_cred_option = SO_PEERCRED;
typedef struct sockpeercred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.uid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.gid; }

# elif defined (LOCAL_PEEREID) && defined(__NetBSD__)
constexpr int socket_cred_option = LOCAL_PEEREID;
typedef struct unpcbid cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.unp_pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.unp_euid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.unp_egid; }

# else
#  error UNRECOGNIZED PLATFORM.  Please submit a patch!
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


#elif defined(SCM_CREDENTIALS) || defined (SCM_CREDS)

# if defined (SCM_CREDENTIALS) && defined(__linux__)
constexpr int credential_message = SCM_CREDENTIALS;
typedef struct ucred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.uid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.gid; }

# elif defined (SCM_CREDS) && defined(__NetBSD__)
constexpr int credential_message = SCM_CREDS;
typedef struct cred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.sc_pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.sc_euid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.sc_egid; }

# elif defined (SCM_CREDS) && \
  (defined(__FreeBSD__) /* FreeBSD */ || \
  (defined(__APPLE__) && defined(__MACH__)) /* Darwin */ )
constexpr int credential_message = SCM_CREDS;
typedef struct cmsgcred cred_t;
constexpr pid_t peer_pid(const cred_t& data) { return data.cmcred_pid; }
constexpr uid_t peer_uid(const cred_t& data) { return data.cmcred_euid; }
constexpr gid_t peer_gid(const cred_t& data) { return data.cmcred_groups[0]; }

# else
#  error UNRECOGNIZED PLATFORM.  Please submit a patch!
# endif


int recv_cred(int socket, proccred_t& cred) noexcept
{

  return posix::success_response;
}


int send_cred(int socket) noexcept
{
  struct sockaddr_storage sockaddr;
  struct sockaddr_un* addr;
  struct msghdr msg;
  struct cmsghdr* cmsg_header;
  struct iovec iov[1];
  ssize_t nbytes;
  int i;
  char buf[CMSG_SPACE(sizeof(cred_t))], c;

  addr = (struct sockaddr_un *)&sockaddr;
  addr->sun_family = AF_LOCAL;
  strlcpy(addr->sun_path, sockpath, sizeof(addr->sun_path));
  addr->sun_len = SUN_LEN(addr);

  c = '*';
  iov[0].iov_base = &c;
  iov[0].iov_len = sizeof(c);
  memset(buf, 0x0b, sizeof(buf));
  cmsg_header = (struct cmsghdr *)buf;
  cmsg_header->cmsg_len = CMSG_LEN(sizeof(cred_t));
  cmsg_header->cmsg_level = SOL_SOCKET;
  cmsg_header->cmsg_type = credential_message;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = iov;
  msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
  msg.msg_control = cmsg_header;
  msg.msg_controllen = CMSG_SPACE(sizeof(cred_t));
  msg.msg_flags = 0;

  return ::sendmsg(fd, &msg, 0);
}


#elif defined(__unix__)

# error no code yet for your operating system. :(

#else
# error Unsupported platform! >:(
#endif
