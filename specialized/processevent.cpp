#include "processevent.h"

// PUT
#include <put/specialized/osdetect.h>
#include <put/specialized/eventbackend.h>

#if defined(FORCE_PROCESS_POLLING)
# pragma message("Forcing use of polling /proc to detect process events.")
# define FALLBACK_ON_PROCESS_POLLING

#elif defined(__linux__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,6,15) /* Linux 2.6.15+ */

// Linux
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>

// PUT
# include <put/cxxutils/posix_helpers.h>
# include <put/cxxutils/socket_helpers.h>
# include <put/cxxutils/vterm.h>

// process flags
static constexpr uint8_t from_native_flags(const native_flags_t flags) noexcept
{
  return
      (flags & proc_event::PROC_EVENT_EXEC ? ProcessEvent::Exec : 0) |
      (flags & proc_event::PROC_EVENT_EXIT ? ProcessEvent::Exit : 0) |
      (flags & proc_event::PROC_EVENT_FORK ? ProcessEvent::Fork : 0) ;
}
/*
static constexpr native_flags_t to_native_flags(const uint8_t flags) noexcept
{
  return
      (flags & ProcessEvent::Exec ? native_flags_t(proc_event::PROC_EVENT_EXEC) : 0) | // Process called exec*()
      (flags & ProcessEvent::Exit ? native_flags_t(proc_event::PROC_EVENT_EXIT) : 0) | // Process exited
      (flags & ProcessEvent::Fork ? native_flags_t(proc_event::PROC_EVENT_FORK) : 0) ; // Process forked
}
*/

struct ProcessEvent::platform_dependant // process notification (process events connector)
{
  enum {
    Read = 0,
    Write = 1,
  };

  bool permitted;
  posix::fd_t fd;
  struct eventinfo_t
  {
    posix::fd_t fd[2]; // two fds for pipe based communication
    Flags_t flags;
  };

  std::unordered_map<pid_t, eventinfo_t> events;

  platform_dependant(void) noexcept
  {
    fd = posix::socket(EDomain::netlink, EType::datagram, EProtocol::connector);
    flaw(fd == posix::invalid_descriptor,
         terminal::warning,,,
         "Unable to open a netlink socket for Process Events Connector: %s", posix::strerror(errno))

    sockaddr_nl sa_nl;
    sa_nl.nl_family = PF_NETLINK;
    sa_nl.nl_groups = CN_IDX_PROC;
    sa_nl.nl_pid = uint32_t(getpid());

    permitted = posix::bind(fd, reinterpret_cast<struct sockaddr *>(&sa_nl), sizeof(sa_nl));
    if(!permitted)
    {
      flaw(errno != posix::errc::operation_not_permitted,
           terminal::warning,,,
           "Unable to bind socket for Process Events Connector: %s", posix::strerror(errno))
      errno = posix::success_response; // clear error
    }
    else
    {
#pragma pack(push, 1)
      struct alignas(NLMSG_ALIGNTO) procconn_t // 32-bit alignment
      {
        nlmsghdr header; // 16 bytes
        cn_msg message;
        proc_cn_mcast_op operation;
      } procconn;
#pragma pack(pop)
      static_assert(sizeof(nlmsghdr) + sizeof(cn_msg) + sizeof(proc_cn_mcast_op) == sizeof(procconn_t), "compiler needs to pack struct");

      posix::memset(&procconn, 0, sizeof(procconn));
      procconn.header.nlmsg_len = sizeof(procconn);
      procconn.header.nlmsg_pid = uint32_t(getpid());
      procconn.header.nlmsg_type = NLMSG_DONE;
      procconn.message.id.idx = CN_IDX_PROC;
      procconn.message.id.val = CN_VAL_PROC;
      procconn.message.len = sizeof(proc_cn_mcast_op);
      procconn.operation = PROC_CN_MCAST_LISTEN;

      flaw(posix::send(fd, &procconn, sizeof(procconn)) == posix::error_response,
           terminal::warning,,,
           "Failed to enable Process Events Connector notifications: %s", posix::strerror(errno));

      EventBackend::add(fd, EventBackend::SimplePollReadFlags,
                        [this](posix::fd_t lambda_fd, native_flags_t) noexcept { read(lambda_fd); });

      posix::fprintf(stderr, "%s%s\n", terminal::information, "Process Events Connector active");
    }
  }

  ~platform_dependant(void) noexcept
  {
    if(permitted)
      EventBackend::remove(fd, EventBackend::SimplePollReadFlags);
    posix::close(fd);
    fd = posix::invalid_descriptor;
  }

  posix::fd_t add(pid_t pid, Flags_t flags) noexcept
  {
    if(events.find(pid) != events.end())
      return events.find(pid)->second.fd[Read];

    eventinfo_t data;
    data.flags = flags;
    if(!permitted || !posix::pipe(data.fd))
      return posix::invalid_descriptor;

    posix::fcntl(data.fd[Read ], F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::fcntl(data.fd[Write], F_SETFD, FD_CLOEXEC); // close on exec*()

    auto iter = events.emplace(pid, data);

    // add Berkeley Packet Filter installation code here

    return data.fd[Read];
  }

  bool remove(pid_t pid) noexcept
  {
    auto iter = events.find(pid);
    if(!permitted || iter == events.end())
      return false;

    // add Berkeley Packet Filter removal code here

    posix::close(iter->second.fd[Read]);
    posix::close(iter->second.fd[Write]);
    events.erase(iter);
    return true;
  }

  void read(posix::fd_t procfd) noexcept
  {
#pragma pack(push, 1)
    struct alignas(NLMSG_ALIGNTO) procmsg_t // 32-bit alignment
    {
      nlmsghdr header; // 16 bytes
      cn_msg message;
      proc_event event;
    } procmsg;
#pragma pack(pop)
    static_assert(sizeof(nlmsghdr) + sizeof(cn_msg) + sizeof(proc_event) == sizeof(procmsg_t), "compiler needs to pack struct");

    pollfd fds = { procfd, POLLIN, 0 };
    while(posix::poll(&fds, 1, 0) > 0 && // while there are messages AND
          posix::recv(procfd, reinterpret_cast<void*>(&procmsg), sizeof(procmsg_t), 0) > 0) // read process event message
    {
      auto iter = events.find(procmsg.event.event_data.id.process_pid); // find event info for this PID
      if(iter != events.end()) // if found...
        posix::write(iter->second.fd[Write], &procmsg.event, sizeof(procmsg.event)); // write process event info into the communications pipe
    }
  }
} ProcessEvent::s_platform;

ProcessEvent::ProcessEvent(pid_t _pid, Flags_t _flags) noexcept
  : m_pid(_pid), m_flags(_flags), m_fd(posix::invalid_descriptor)
{
  m_fd = s_platform.add(m_pid, m_flags); // add PID to monitor and return communications pipe
  if(m_fd != posix::invalid_descriptor)
    EventBackend::add(m_fd, EventBackend::SimplePollReadFlags, // connect communications pipe to a lambda function
                      [this](posix::fd_t lambda_fd, native_flags_t) noexcept
                      {
                        proc_event data;
                        pollfd fds = { lambda_fd, POLLIN, 0 };
                        while(posix::poll(&fds, 1, 0) > 0 && // while there is another event to be read
                              posix::read(lambda_fd, &data, sizeof(data)) > 0) // read the event
                          switch(from_native_flags(data.what)) // find the type of event
                          {
                            case Flags::Exec: // queue exec signal with PID
                              Object::enqueue(execed,
                                              data.event_data.exec.process_pid);
                              break;
                            case Flags::Exit: // queue exit signal with PID and exit code
                              if(data.event_data.exit.exit_signal) // if killed by a signal
                                Object::enqueue(killed,
                                                data.event_data.exit.process_pid,
                                                *reinterpret_cast<posix::Signal::EId*>(&data.event_data.exit.exit_signal));
                              else // else exited by itself
                                Object::enqueue(exited,
                                                data.event_data.exit.process_pid,
                                                *reinterpret_cast<posix::error_t*>(&data.event_data.exit.exit_code));
                              break;
                            case Flags::Fork: // queue fork signal with PID and child PID
                              Object::enqueue(forked,
                                              data.event_data.fork.parent_pid,
                                              data.event_data.fork.child_pid);
                              break;
                          }
                      });
}

ProcessEvent::~ProcessEvent(void) noexcept
{
  if(m_fd != posix::invalid_descriptor)
  {
    EventBackend::remove(m_fd, EventBackend::SimplePollReadFlags);
    s_platform.remove(m_pid);
  }
}

#elif defined(__darwin__)     /* Darwin 7+     */ || \
      defined(__DragonFly__)  /* DragonFly BSD */ || \
      (defined(__FreeBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(4,1,0))  /* FreeBSD 4.1+  */ || \
      (defined(__OpenBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,9,0))  /* OpenBSD 2.9+  */ || \
      (defined(__NetBSD__)  && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,0,0))  /* NetBSD 2+     */

#include <sys/event.h> // kqueue

static constexpr native_flags_t composite_flag(uint16_t actions, int16_t filters, uint32_t flags) noexcept
  { return native_flags_t(actions) | (native_flags_t(uint16_t(filters)) << 16) | (native_flags_t(flags) << 32); }

static constexpr bool flag_subset(native_flags_t flags, native_flags_t subset)
  { return (flags & subset) == subset; }

static constexpr native_flags_t to_native_flags(const uint8_t flags) noexcept
{
  return
      (flags & ProcessEvent::Exec ? composite_flag(0, EVFILT_PROC, NOTE_EXEC) : 0) |
      (flags & ProcessEvent::Exit ? composite_flag(0, EVFILT_PROC, NOTE_EXIT) : 0) |
      (flags & ProcessEvent::Fork ? composite_flag(0, EVFILT_PROC, NOTE_FORK) : 0) ;
}

static constexpr ushort extract_filter(native_flags_t flags) noexcept
  { return (flags >> 16) & 0xFFFF; }

template<typename rtype>
static constexpr rtype extract_flags(native_flags_t flags) noexcept
  { return flags >> 32; }

ProcessEvent::ProcessEvent(pid_t _pid, Flags_t _flags) noexcept
  : m_pid(_pid), m_flags(_flags), m_fd(posix::invalid_descriptor)
{
  EventBackend::add(m_pid, to_native_flags(m_flags), // connect PID event to lambda function
                    [this](posix::fd_t lambda_fd, native_flags_t lambda_flags) noexcept
                    {
                      switch(extract_filter(lambda_flags)) // switch by filtered event type
                      {
                        case Flags::Exec: // queue exec signal with PID
                          Object::enqueue_copy(execed, m_pid);
                          break;
                        case Flags::Exit: // queue exit signal with PID and exit code
                          Object::enqueue_copy(exited, m_pid, extract_flags<posix::error_t>(lambda_flags));
                          break;
                        case Flags::Fork: // queue fork signal with PID and child PID
                          Object::enqueue_copy(forked, m_pid, extract_flags<pid_t>(lambda_flags));
                          break;
                      }
                    });
}

ProcessEvent::~ProcessEvent(void) noexcept
{
  EventBackend::remove(m_pid, to_native_flags(m_flags)); // disconnect PID with flags
}

#elif defined(__solaris__) /* Solaris */
# pragma message("No process event backend code exists in PUT for Solaris!  Please submit a patch!")
# define FALLBACK_ON_PROCESS_POLLING

#elif defined(__QNX__) // QNX
// QNX docs: http://www.qnx.com/developers/docs/7.0.0/index.html#com.qnx.doc.neutrino.devctl/topic/about.html
# pragma message("No process event backend code exists in PUT for QNX!  Please submit a patch!")
# define FALLBACK_ON_PROCESS_POLLING

#else
# pragma message("No platform specific process event detection code! Falling back on polling /proc.")
# define FALLBACK_ON_PROCESS_POLLING
#endif

#if defined(FALLBACK_ON_PROCESS_POLLING)
#error TODO: implement process event detection code by polling /proc.
#endif
