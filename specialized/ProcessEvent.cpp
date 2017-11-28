#include "ProcessEvent.h"

#if defined(__linux__) /* Linux 2.6.15+ */

// Linux
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>

// PDTK
#include <cxxutils/posix_helpers.h>
#include <cxxutils/socket_helpers.h>
#include <cxxutils/vterm.h>

#include <cassert>

// process flags
static constexpr uint8_t from_native_flags(const native_flags_t flags) noexcept
{
  return
      (flags & proc_event::PROC_EVENT_EXEC ? ProcessEvent::Exec : 0) |
      (flags & proc_event::PROC_EVENT_EXIT ? ProcessEvent::Exit : 0) |
      (flags & proc_event::PROC_EVENT_FORK ? ProcessEvent::Fork : 0) ;
}

static constexpr native_flags_t to_native_flags(const uint8_t flags) noexcept
{
  return
      (flags & ProcessEvent::Exec ? native_flags_t(proc_event::PROC_EVENT_EXEC) : 0) | // Process called exec*()
      (flags & ProcessEvent::Exit ? native_flags_t(proc_event::PROC_EVENT_EXIT) : 0) | // Process exited
      (flags & ProcessEvent::Fork ? native_flags_t(proc_event::PROC_EVENT_FORK) : 0) ; // Process forked
}

struct ProcessEvent::platform_dependant // process notification (process events connector)
{
  posix::fd_t fd;
  std::unordered_multimap<pid_t, ProcessEvent::Flags_t> events;

  platform_dependant(void) noexcept
    : fd(posix::invalid_descriptor)
  {
    fd = posix::socket(EDomain::netlink, EType::datagram, EProtocol::connector);
    flaw(fd == posix::invalid_descriptor, terminal::warning,,,
         "Unable to open a netlink socket for Process Events Connector: %s", std::strerror(errno))

    sockaddr_nl sa_nl;
    sa_nl.nl_family = PF_NETLINK;
    sa_nl.nl_groups = CN_IDX_PROC;
    sa_nl.nl_pid = getpid();

    flaw(!posix::bind(fd, reinterpret_cast<struct sockaddr *>(&sa_nl), sizeof(sa_nl)), terminal::warning,,,
         "Process Events Connector requires root level access: %s", std::strerror(errno))

    struct alignas(NLMSG_ALIGNTO) // 32-bit alignment
    {
      nlmsghdr header; // 16 bytes
      struct __attribute__((__packed__))
      {
        cn_msg message;
        proc_cn_mcast_op operation;
      };
    } procconn;
    static_assert(sizeof(nlmsghdr) + sizeof(cn_msg) + sizeof(proc_cn_mcast_op) == sizeof(procconn), "compiler failed to pack struct");

    std::memset(&procconn, 0, sizeof(procconn));
    procconn.header.nlmsg_len = sizeof(procconn);
    procconn.header.nlmsg_pid = getpid();
    procconn.header.nlmsg_type = NLMSG_DONE;
    procconn.message.id.idx = CN_IDX_PROC;
    procconn.message.id.val = CN_VAL_PROC;
    procconn.message.len = sizeof(proc_cn_mcast_op);
    procconn.operation = PROC_CN_MCAST_LISTEN;

    flaw(posix::send(fd, &procconn, sizeof(procconn)) == posix::error_response, terminal::warning,,,
         "Failed to enable Process Events Connector notifications: %s", std::strerror(errno))
  }

  ~platform_dependant(void) noexcept
  {
    posix::close(fd);
    fd = posix::invalid_descriptor;
  }

  posix::fd_t add(pid_t pid, ProcessEvent::Flags_t flags) noexcept
  {
    auto iter = events.emplace(pid, flags);

    // add filter installation code here

    return iter->first;
  }

  bool remove(pid_t pid) noexcept
  {
    if(!events.erase(pid)) // erase all the entries for that PID
      return false; // no entries found

    // add filter removal code here

    return true;
  }

  struct return_data
  {
    pid_t pid;
    ProcessEvent::Flags_t flags;
  };

  return_data read(posix::fd_t procfd) noexcept
  {
    struct alignas(NLMSG_ALIGNTO) // 32-bit alignment
    {
      nlmsghdr header; // 16 bytes
      struct __attribute__((__packed__))
      {
        cn_msg message;
        proc_event event;
      };
    } procmsg;

    pollfd fds = { procfd, POLLIN, 0 };
    while(posix::poll(&fds, 1, 0) > 0) // while there are messages
    {
      if(posix::recv(fd, reinterpret_cast<void*>(&procmsg), sizeof(procmsg), 0) > 0) // read process event message
        return {fd, from_native_flags(procmsg.event.what) };
    }
    assert(false);
    return {0, 0};
  }
} ProcessEvent::s_platform;

ProcessEvent::ProcessEvent(pid_t _pid, Flags_t _flags) noexcept
  : m_pid(_pid), m_flags(_flags), m_fd(posix::invalid_descriptor)
{
  m_fd = s_platform.add(m_pid, m_flags);
  EventBackend::add(m_fd, EventBackend::SimplePollReadFlags,
                    [this](posix::fd_t lambda_fd, native_flags_t) noexcept
                    {
                      platform_dependant::return_data data = s_platform.read(lambda_fd);
                      assert(m_pid == data.pid);
                      assert(m_flags & data.flags);
                      Object::enqueue(activated, data.pid, data.flags);
                    });
}

ProcessEvent::~ProcessEvent(void) noexcept
{
  EventBackend::remove(m_fd, EventBackend::SimplePollReadFlags);
  assert(s_platform.remove(m_pid));
}

#elif (defined(__APPLE__) && defined(__MACH__)) /* Darwin 7+     */ || \
      defined(__FreeBSD__)                      /* FreeBSD 4.1+  */ || \
      defined(__DragonFly__)                    /* DragonFly BSD */ || \
      defined(__OpenBSD__)                      /* OpenBSD 2.9+  */ || \
      defined(__NetBSD__)                       /* NetBSD 2+     */

#include <sys/event.h> // kqueue

static constexpr native_flags_t composite_flag(uint16_t actions, int16_t filters, uint32_t flags) noexcept
  { return native_flags_t(actions) | (native_flags_t(uint16_t(filters)) << 16) | (native_flags_t(flags) << 32); }

static constexpr bool flag_subset(native_flags_t flags, native_flags_t subset)
  { return (flags & subset) == subset; }

// process flags
static constexpr uint8_t from_native_flags(const native_flags_t flags) noexcept
{
  return
      (flag_subset(flags, composite_flag(0, EVFILT_PROC, NOTE_EXEC)) ? ProcessEvent::Exec : 0) |
      (flag_subset(flags, composite_flag(0, EVFILT_PROC, NOTE_EXIT)) ? ProcessEvent::Exit : 0) |
      (flag_subset(flags, composite_flag(0, EVFILT_PROC, NOTE_FORK)) ? ProcessEvent::Fork : 0) ;
}

static constexpr native_flags_t to_native_flags(const uint8_t flags) noexcept
{
  return
      (flags & ProcessEvent::Exec ? composite_flag(0, EVFILT_PROC, NOTE_EXEC) : 0) |
      (flags & ProcessEvent::Exit ? composite_flag(0, EVFILT_PROC, NOTE_EXIT) : 0) |
      (flags & ProcessEvent::Fork ? composite_flag(0, EVFILT_PROC, NOTE_FORK) : 0) ;
}

ProcessEvent::ProcessEvent(pid_t _pid, Flags_t _flags) noexcept
  : m_pid(_pid), m_flags(_flags), m_fd(posix::invalid_descriptor)
{
  EventBackend::add(m_pid, to_native_flags(m_flags), // connect FD with flags to signal
                    [this](posix::fd_t lambda_fd, native_flags_t lambda_flags) noexcept
                    { Object::enqueue_copy<pid_t, Flags_t>(activated, m_pid, from_native_flags(lambda_flags)); });
}

ProcessEvent::~ProcessEvent(void) noexcept
{
  EventBackend::remove(m_pid, to_native_flags(m_flags)); // disconnect FD with flags from signal
}

#elif defined(__sun) && defined(__SVR4) // Solaris / OpenSolaris / OpenIndiana / illumos
# error No process event backend code exists in PDTK for Solaris / OpenSolaris / OpenIndiana / illumos!  Please submit a patch!

#elif defined(__minix) // MINIX
# error No process event backend code exists in PDTK for MINIX!  Please submit a patch!

#elif defined(__QNX__) // QNX
// QNX docs: http://www.qnx.com/developers/docs/7.0.0/index.html#com.qnx.doc.neutrino.devctl/topic/about.html
# error No process event backend code exists in PDTK for QNX!  Please submit a patch!

#elif defined(__hpux) // HP-UX
# error No process event backend code exists in PDTK for HP-UX!  Please submit a patch!

#elif defined(_AIX) // IBM AIX
# error No process event backend code exists in PDTK for IBM AIX!  Please submit a patch!

#elif defined(BSD)
# error Unrecognized BSD derivative!

#elif defined(__unix__) || defined(__unix)
# error Unrecognized UNIX variant!

#else
# error This platform is not supported.
#endif
