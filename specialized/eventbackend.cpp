#include "eventbackend.h"

#if defined(__linux__)

// Linux
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <linux/connector.h>
#include <linux/netlink.h>
#include <linux/cn_proc.h>

// C++
#include <cassert>
#include <climits>
#include <cstring>
#include <unordered_map>
#include <algorithm>

// POSIX
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 2048

// FD flags
inline EventData_t from_native_fdflags(const uint32_t flags) noexcept
{
  EventData_t data;
  data.flags.Error        = flags & EPOLLERR ? 1 : 0;
  data.flags.Disconnected = flags & EPOLLHUP ? 1 : 0;
  data.flags.Readable     = flags & EPOLLIN  ? 1 : 0;
  data.flags.Writeable    = flags & EPOLLOUT ? 1 : 0;
  data.flags.EdgeTrigger  = flags & EPOLLET  ? 1 : 0;
  return data;
}

constexpr uint32_t to_native_fdflags(const EventFlags_t& flags)
{
  return
      (flags.Error        ? uint32_t(EPOLLERR) : 0) |
      (flags.Disconnected ? uint32_t(EPOLLHUP) : 0) |
      (flags.Readable     ? uint32_t(EPOLLIN ) : 0) |
      (flags.Writeable    ? uint32_t(EPOLLOUT) : 0) |
      (flags.EdgeTrigger  ? uint32_t(EPOLLET ) : 0) ;
}

// file/directory flags
inline EventData_t from_native_fileflags(const uint32_t flags) noexcept
{
  EventData_t data;
  data.flags.ReadEvent    = flags & IN_ACCESS    ? 1 : 0;
  data.flags.WriteEvent   = flags & IN_MODIFY    ? 1 : 0;
  data.flags.AttributeMod = flags & IN_ATTRIB    ? 1 : 0;
  data.flags.Moved        = flags & IN_MOVE_SELF ? 1 : 0;
  return data;
}

constexpr uint32_t to_native_fileflags(const EventFlags_t& flags)
{
  return
      (flags.ReadEvent    ? uint32_t(IN_ACCESS   ) : 0) | // File was accessed (read) (*).
      (flags.WriteEvent   ? uint32_t(IN_MODIFY   ) : 0) | // File was modified (*).
      (flags.AttributeMod ? uint32_t(IN_ATTRIB   ) : 0) | // Metadata changed, e.g., permissions, timestamps, extended attributes, link count (since Linux 2.6.25), UID, GID, etc. (*).
      (flags.Moved        ? uint32_t(IN_MOVE_SELF) : 0) ; // Watched file/directory was itself moved.
}

// process flags
inline EventFlags_t from_native_procflags(const uint32_t flags) noexcept
{
  EventFlags_t rval;
  rval.ExecEvent = flags & proc_event::PROC_EVENT_EXEC ? 1 : 0;
  rval.ExitEvent = flags & proc_event::PROC_EVENT_EXIT ? 1 : 0;
  rval.ForkEvent = flags & proc_event::PROC_EVENT_FORK ? 1 : 0;
  rval.UIDEvent  = flags & proc_event::PROC_EVENT_UID  ? 1 : 0;
  rval.GIDEvent  = flags & proc_event::PROC_EVENT_GID  ? 1 : 0;
  rval.SIDEvent  = flags & proc_event::PROC_EVENT_SID  ? 1 : 0;
  return rval;
}

constexpr uint32_t to_native_procflags(const EventFlags_t& flags)
{
  return
      (flags.ExecEvent ? uint32_t(proc_event::PROC_EVENT_EXEC) : 0) | // Process called exec*()
      (flags.ExitEvent ? uint32_t(proc_event::PROC_EVENT_EXIT) : 0) | // Process exited
      (flags.ForkEvent ? uint32_t(proc_event::PROC_EVENT_FORK) : 0) | // Process forked
      (flags.UIDEvent  ? uint32_t(proc_event::PROC_EVENT_UID ) : 0) | // Process changed it's User ID
      (flags.GIDEvent  ? uint32_t(proc_event::PROC_EVENT_GID ) : 0) | // Process changed it's Group ID
      (flags.SIDEvent  ? uint32_t(proc_event::PROC_EVENT_SID ) : 0);  // Process changed it's Session ID
}

struct proc_fd
{
  uint32_t msb : 1;
  uint32_t flags_upper : 2;
  uint32_t flags_lower : 13;
  uint32_t pid : 16;

  proc_fd(const uint32_t val) noexcept
    { *reinterpret_cast<uint32_t*>(this) = val; }

  proc_fd(const uint32_t _pid, const EventFlags_t& flags) noexcept
    : msb(1), pid(_pid)
    { write_flags(flags); }

  EventFlags_t read_flags(void) const noexcept
    { return from_native_procflags((flags_upper << 30) | flags_lower); }

  void write_flags(const EventFlags_t& flags) noexcept
  {
    uint32_t f = to_native_procflags(flags);
    flags_upper = f >> 30;
    flags_lower = f;
  }

  operator posix::fd_t(void) const noexcept
    { return *reinterpret_cast<const posix::fd_t*>(this); }
};

struct platform_dependant
{
  struct pollnotify_t // poll notification (epoll)
  {
    posix::fd_t fd;
    struct epoll_event output[MAX_EVENTS];
    int output_count;

    pollnotify_t(void) noexcept
    {
      fd = epoll_create(MAX_EVENTS);
      assert(fd != posix::error_response);
      output_count = 0;
    }

    ~pollnotify_t(void) noexcept
    {
      ::close(fd);
      fd = posix::error_response;
    }

  } pollnotify;

  struct fsnotify_t // file notification (inotify)
  {
    posix::fd_t fd;
    std::set<posix::fd_t> fds;

    fsnotify_t(void) noexcept
    {
      fd = inotify_init();
      assert(fd != posix::error_response);
    }

    ~fsnotify_t(void) noexcept
    {
      ::close(fd);
      fd = posix::error_response;
    }

    posix::fd_t watch(const char* path, EventFlags_t flags) noexcept
    {
      posix::fd_t wd = inotify_add_watch(fd, path, to_native_fileflags(flags));
      if(wd < 0)
        return wd;
      fds.emplace(wd);
      return wd;
    }

    bool remove(posix::fd_t wd) noexcept
    {
      if(!fds.erase(wd)) // if erased zero
        return false;
      return inotify_rm_watch(fd, wd) == posix::success_response;
    }
  } fsnotify;

  struct procnotify_t // process notification (process events connector)
  {
    posix::fd_t fd;
    std::unordered_multimap<pid_t, proc_fd> events;

    procnotify_t(void) noexcept
    {
      fd = ::socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
      assert(fd != posix::error_response);
      sockaddr_nl sa_nl;
      sa_nl.nl_family = AF_NETLINK;
      sa_nl.nl_groups = CN_IDX_PROC;
      sa_nl.nl_pid = getpid();
      int binderr = ::bind(fd, (struct sockaddr *)&sa_nl, sizeof(sa_nl));
      if(binderr == posix::error_response)
        perror("Process Events Connector requires root level access");
      else
      {
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

        if(::send(fd, &procconn, sizeof(procconn), 0) == posix::error_response)
          perror("Failed to enable Process Events Connector notifications");
      }
    }

    ~procnotify_t(void) noexcept
    {
      ::close(fd);
      fd = posix::error_response;
    }

    posix::fd_t watch(pid_t pid, EventFlags_t flags) noexcept
    {
      auto iter = events.emplace(pid, proc_fd(pid, flags));

      // add filter installation code here

      return iter->second;
    }

    bool remove(posix::fd_t _fd) noexcept
    {
      if(!events.erase(proc_fd(_fd).pid))
        return false;

      // add filter removal code here

      return true;
    }
  } procnotify;

};

std::multimap<posix::fd_t, EventFlags_t> EventBackend::queue; // watch queue
std::multimap<posix::fd_t, EventData_t> EventBackend::results;

struct platform_dependant* EventBackend::platform = nullptr;

void EventBackend::init(void) noexcept
{
  assert(platform == nullptr);
  platform = new platform_dependant;
  watch(platform->procnotify.fd, EventFlags::Readable);
}

void EventBackend::destroy(void) noexcept
{
  assert(platform != nullptr);
  delete platform;
  platform = nullptr;
}


posix::fd_t EventBackend::watch(const char* path, EventFlags_t flags) noexcept
{
  posix::fd_t fd = platform->fsnotify.watch(path, flags);
  if(fd > 0 && watch(fd, EventFlags::Readable) <= 0)
  {
    platform->fsnotify.remove(fd);
    fd = posix::error_response;
  }
  return fd;
}

posix::fd_t EventBackend::watch(int target, EventFlags_t flags) noexcept
{
  posix::fd_t fd = 0;
  if(flags >= EventFlags::ExecEvent)
  {
    fd = platform->procnotify.watch(target, flags);
    flags = EventFlags::Readable;
  }
  else
    fd = target;

  struct epoll_event native_event;
  native_event.data.fd = fd;
  native_event.events = to_native_fdflags(flags); // be sure to convert to native events
  auto iter = queue.find(fd); // search queue for FD
  if(iter != queue.end()) // entry exists for FD
  {
    if(epoll_ctl(platform->pollnotify.fd, EPOLL_CTL_MOD, fd, &native_event) == posix::success_response || // try to modify FD first
       (errno == ENOENT && epoll_ctl(platform->pollnotify.fd, EPOLL_CTL_ADD, fd, &native_event) == posix::success_response)) // error: FD not added, so try adding
    {
      iter->second = flags; // set value of existing entry
      posix::success();
    }
  }
  else if(epoll_ctl(platform->pollnotify.fd, EPOLL_CTL_ADD, fd, &native_event) == posix::success_response || // try adding FD first
          (errno == EEXIST && epoll_ctl(platform->pollnotify.fd, EPOLL_CTL_MOD, fd, &native_event) == posix::success_response)) // error: FD already added, so try modifying
  {
    queue.emplace(fd, flags); // create a new entry
    posix::success();
  }

  return fd;
}

bool EventBackend::remove(posix::fd_t fd) noexcept
{
  struct epoll_event event;
  auto iter = queue.find(fd); // search queue for FD
  if(iter == queue.end() && // entry exists for FD
     epoll_ctl(platform->pollnotify.fd, EPOLL_CTL_DEL, fd, &event) == posix::success_response) // try to delete entry
    queue.erase(iter); // remove entry for FD
  return errno == posix::success_response;
}

#define INOTIFY_EVENT_SIZE   (sizeof(inotify_event) + NAME_MAX + 1)

bool EventBackend::getevents(int timeout) noexcept
{
  platform->pollnotify.output_count = epoll_wait(platform->pollnotify.fd, platform->pollnotify.output, MAX_EVENTS, timeout); // wait for new results
  results.clear(); // clear old results

  if(platform->pollnotify.output_count == posix::error_response) // if error/timeout occurred
    return false; //fail

  uint8_t inotifiy_buffer_data[INOTIFY_EVENT_SIZE * 16]; // queue has a minimum of size of 16 inotify events

  const epoll_event* end = platform->pollnotify.output + platform->pollnotify.output_count;
  for(epoll_event* pos = platform->pollnotify.output; pos != end; ++pos) // iterate through results
  {
    if(pos->data.fd == platform->procnotify.fd) // if a process event
    {
      struct alignas(NLMSG_ALIGNTO) // 32-bit alignment
      {
        nlmsghdr header; // 16 bytes
        struct __attribute__((__packed__))
        {
          cn_msg message;
          proc_event event;
        };
      } procnote;

      if(::recv(platform->procnotify.fd, &procnote, sizeof(procnote), 0) > 0)
      {
        EventFlags_t flags = from_native_procflags(procnote.event.what);
        auto entries = platform->procnotify.events.equal_range(procnote.event.event_data.id.process_pid); // get all the entries for that PID
        for_each(entries.first, entries.second, // for each matching PID entry
          [procnote, flags](const std::pair<pid_t, proc_fd>& pair)
          {
            if(pair.second.read_flags() & flags) // test to see if the current process matches the triggering EventFlag
              results.emplace(pair.second,
                EventData_t(flags,
                 procnote.event.event_data.exit.process_pid,
                 procnote.event.event_data.exit.process_tgid,
                 procnote.event.event_data.exit.exit_code,
                 procnote.event.event_data.exit.exit_signal));
          });
      }
    }
    else if(platform->fsnotify.fds.find(pos->data.fd) != platform->fsnotify.fds.end()) // if an inotify event
    {
      union {
        uint8_t* inpos;
        inotify_event* incur;
      };
      uint8_t* inend = inotifiy_buffer_data + posix::read(pos->data.fd, inotifiy_buffer_data, sizeof(inotifiy_buffer_data)); // read data and locate it's end
      for(inpos = inotifiy_buffer_data; inpos < inend; inpos += sizeof(inotify_event) + incur->len) // iterate through the inotify events
        results.emplace(static_cast<posix::fd_t>(pos->data.fd), from_native_fileflags(incur->mask)); // save result (in non-native format)
    }
    else // normal file descriptor event
      results.emplace(posix::fd_t(pos->data.fd), from_native_fdflags(pos->events)); // save result (in non-native format)
  }
  return true;
}
#elif defined(__unix__)

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif
