#include "eventbackend.h"

#define MAX_EVENTS 1024

#if defined(__linux__)

// epoll needs kernel 2.5.44
// "process events connector" needs  kernel 2.6.15
// uevents needs kernel 2.6.10
// eventfd needs kernel 2.6.22

// Linux
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <linux/netlink.h>
#ifdef GLOBAL_PROCESS_EVENT_TRACKING
# include <linux/connector.h>
# include <linux/cn_proc.h>
#endif

// STL
#include <vector>
#include <algorithm>
#include <string>
#include <unordered_set>

// POSIX
#include <sys/socket.h>
#include <unistd.h>

// POSIX++
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cstring>

// PDTK
#include <cxxutils/vterm.h>
#include <cxxutils/error_helpers.h>
#include <cxxutils/socket_helpers.h>


struct platform_dependant
{
  struct pollnotify_t // poll notification (epoll)
  {
    posix::fd_t fd;
    std::unordered_multimap<posix::fd_t, EventFlags_t>& fds;
    struct epoll_event output[MAX_EVENTS];

    // FD flags
    static EventData_t from_native_flags(const uint32_t flags) noexcept
    {
      EventData_t data;
      data.flags.Error        = flags & EPOLLERR ? 1 : 0;
      data.flags.Disconnected = flags & EPOLLHUP ? 1 : 0;
      data.flags.Readable     = flags & EPOLLIN  ? 1 : 0;
      data.flags.Writeable    = flags & EPOLLOUT ? 1 : 0;
      data.flags.EdgeTrigger  = flags & EPOLLET  ? 1 : 0;
      return data;
    }

    static constexpr uint32_t to_native_flags(const EventFlags_t& flags) noexcept
    {
      return
          (flags.Error        ? uint32_t(EPOLLERR) : 0) |
          (flags.Disconnected ? uint32_t(EPOLLHUP) : 0) |
          (flags.Readable     ? uint32_t(EPOLLIN ) : 0) |
          (flags.Writeable    ? uint32_t(EPOLLOUT) : 0) |
          (flags.EdgeTrigger  ? uint32_t(EPOLLET ) : 0) ;
    }

    pollnotify_t(void) noexcept
      : fd(posix::invalid_descriptor), fds(EventBackend::queue)
    {
      fd = epoll_create(MAX_EVENTS);
      flaw(fd == posix::invalid_descriptor, terminal::critical, std::exit(errno),,
           "Unable to create an instance of epoll! %s", std::strerror(errno))
    }

    ~pollnotify_t(void) noexcept
    {
      posix::close(fd);
      fd = posix::invalid_descriptor;
    }

    int wait(int timeout) noexcept
    {
      return ::epoll_wait(fd, output, MAX_EVENTS, timeout); // wait for new results
    }

    posix::fd_t watch(posix::fd_t wd, EventFlags_t flags) noexcept
    {
      struct epoll_event native_event;
      native_event.data.fd = wd;
      native_event.events = to_native_flags(flags); // be sure to convert to native events
      auto iter = fds.find(wd); // search queue for FD
      if(iter != fds.end()) // entry exists for FD
      {
        if(epoll_ctl(fd, EPOLL_CTL_MOD, wd, &native_event) == posix::success_response || // try to modify FD first
           (errno == ENOENT && epoll_ctl(fd, EPOLL_CTL_ADD, wd, &native_event) == posix::success_response)) // error: FD not added, so try adding
        {
          iter->second = flags; // set value of existing entry
          posix::success();
        }
      }
      else if(epoll_ctl(fd, EPOLL_CTL_ADD, wd, &native_event) == posix::success_response || // try adding FD first
              (errno == EEXIST && epoll_ctl(fd, EPOLL_CTL_MOD, wd, &native_event) == posix::success_response)) // error: FD already added, so try modifying
      {
        fds.emplace(wd, flags); // create a new entry
        posix::success();
      }
      return wd;
    }

    bool remove(posix::fd_t wd) noexcept
    {
      struct epoll_event event;
      auto iter = fds.find(wd); // search queue for FD
      if(iter == fds.end() && // entry exists for FD
         epoll_ctl(fd, EPOLL_CTL_DEL, wd, &event) == posix::success_response) // try to delete entry
      {
        fds.erase(iter); // remove entry for FD
        return true;
      }
      return false;
    }

    void read(posix::fd_t pollfd, uint32_t watchedflags, std::unordered_multimap<posix::fd_t, EventData_t>& results)
    {
      results.emplace(pollfd, from_native_flags(watchedflags)); // save result (in non-native format)
    }

  } pollnotify;

  struct fsnotify_t // file notification (inotify)
  {
    posix::fd_t fd;
    std::unordered_map<std::string, posix::fd_t> path2fds;
    std::unordered_map<posix::fd_t, std::string> fds2path;

    // file/directory flags
    static EventData_t from_native_flags(const uint32_t flags) noexcept
    {
      EventData_t data;
      data.flags.ReadEvent    = flags & IN_ACCESS      ? 1 : 0;
      data.flags.WriteEvent   = flags & IN_MODIFY      ? 1 : 0;
      data.flags.AttributeMod = flags & IN_ATTRIB      ? 1 : 0;
      data.flags.Moved        = flags & IN_MOVE_SELF   ? 1 : 0;
      data.flags.Deleted      = flags & IN_DELETE_SELF ? 1 : 0;
      data.flags.SubCreated   = flags & IN_CREATE      ? 1 : 0;
      data.flags.SubMoved     = flags & IN_MOVE        ? 1 : 0;
      data.flags.SubDeleted   = flags & IN_DELETE      ? 1 : 0;
      return data;
    }

    static constexpr uint32_t to_native_flags(const EventFlags_t& flags) noexcept
    {
      return
          (flags.ReadEvent    ? uint32_t(IN_ACCESS     ) : 0) | // File was accessed (read) (*).
          (flags.WriteEvent   ? uint32_t(IN_MODIFY     ) : 0) | // File was modified (*).
          (flags.AttributeMod ? uint32_t(IN_ATTRIB     ) : 0) | // Metadata changed, e.g., permissions, timestamps, extended attributes, link count (since Linux 2.6.25), UID, GID, etc. (*).
          (flags.Moved        ? uint32_t(IN_MOVE_SELF  ) : 0) | // Watched File was moved.
          (flags.Deleted      ? uint32_t(IN_DELETE_SELF) : 0) | // Watched File was deleted.
          (flags.SubCreated   ? uint32_t(IN_CREATE     ) : 0) | // File created in watched dir.
          (flags.SubMoved     ? uint32_t(IN_MOVE       ) : 0) | // File moved in watched dir.
          (flags.SubDeleted   ? uint32_t(IN_DELETE     ) : 0) ; // File deleted in watched dir.
    }

    fsnotify_t(void) noexcept
      : fd(posix::invalid_descriptor)
    {
      fd = inotify_init();
      flaw(fd == posix::invalid_descriptor, terminal::severe,,,
           "Unable to create an instance of inotify!: %s", std::strerror(errno))
    }

    ~fsnotify_t(void) noexcept
    {
      posix::close(fd);
      fd = posix::invalid_descriptor;
    }

    posix::fd_t watch(const char* path, EventFlags_t flags) noexcept
    {
      posix::fd_t wd = inotify_add_watch(fd, path, to_native_flags(flags));
      if(wd < 0)
        return wd;
      path2fds.emplace(path, wd);
      fds2path.emplace(wd, path);
      return wd;
    }

    posix::fd_t lookup(const char* path)
    {
      auto iter = path2fds.find(path);
      return iter == path2fds.end() ? posix::invalid_descriptor : iter->second;
    }

    bool remove(posix::fd_t wd) noexcept
    {
      auto iter = fds2path.find(wd);
      if(iter == fds2path.end())
        return false;
      path2fds.erase(iter->second);
      fds2path.erase(iter);
      return inotify_rm_watch(fd, wd) == posix::success_response;
    }

    bool owns(posix::fd_t wd) const noexcept
      { return fds2path.find(wd) != fds2path.end(); }

#define INOTIFY_EVENT_SIZE   (sizeof(inotify_event) + NAME_MAX + 1)
    void read(posix::fd_t wd, uint32_t watchedflags, std::unordered_multimap<posix::fd_t, EventData_t>& results)
    {
      (void)watchedflags;
      static uint8_t inotifiy_buffer_data[INOTIFY_EVENT_SIZE * 16]; // queue has a minimum of size of 16 inotify events
      union {
        uint8_t* inpos;
        inotify_event* incur;
      };
      uint8_t* inend = inotifiy_buffer_data + posix::read(wd, inotifiy_buffer_data, sizeof(inotifiy_buffer_data)); // read data and locate it's end
      for(inpos = inotifiy_buffer_data; inpos < inend; inpos += sizeof(inotify_event) + incur->len) // iterate through the inotify events
        results.emplace(wd, from_native_flags(incur->mask)); // save result (in non-native format)
    }

  } fsnotify;
#ifdef MOUNT_NOTIFICATIONS
  struct mountnotify_t // event notifications (via netlink)
  {
    posix::fd_t fd;
    std::unordered_map<std::string, posix::fd_t> mount2fds; // mountpoint/device -> eventfd
    std::unordered_map<posix::fd_t, std::pair<std::string, EventFlags_t>> fds2mount; // eventfd -> mountpoint/device

    mountnotify_t(void) noexcept
      : fd(posix::invalid_descriptor)
    {
      sockaddr_nl sa_nl;
      std::memset(&sa_nl, 0, sizeof(struct sockaddr_nl));
      sa_nl.nl_family = PF_NETLINK;
      sa_nl.nl_groups = UINT32_MAX;
      sa_nl.nl_pid = getpid();

      fd = posix::socket(EDomain::netlink, EType::datagram, EProtocol::uevent);
      flaw(fd == posix::invalid_descriptor, terminal::warning,,,
           "Unable to open a socket for uevent netlink: %s", std::strerror(errno))

      flaw(!posix::bind(fd, (struct sockaddr *)&sa_nl, sizeof(sa_nl)), terminal::warning,,,
           "uevent netlink requires root level access: %s", std::strerror(errno))
    }

    ~mountnotify_t(void) noexcept
    {
      posix::close(fd);
      fd = posix::invalid_descriptor;
    }

    posix::fd_t watch(const char* device, EventFlags_t flags) noexcept
    {
      // add filter installation code here
      posix::fd_t wd = ::eventfd(0, 0);
      fds2mount.emplace(wd, std::make_pair(device, flags));
      mount2fds.emplace(device, wd);
      return wd;
    }

    posix::fd_t lookup(const char* path)
    {
      auto iter = mount2fds.find(path);
      return iter == mount2fds.end() ? posix::invalid_descriptor : iter->second;
    }

    bool remove(posix::fd_t wd) noexcept
    {
      // add filter removal code here
      auto iter = fds2mount.find(wd);
      if(iter == fds2mount.end())
        return false;
      mount2fds.erase(iter->second.first); // erase by name
      fds2mount.erase(iter); // erase iterator
      return posix::close(wd);
    }

    bool owns(posix::fd_t wd) const noexcept
      { return fds2mount.find(wd) != fds2mount.end(); }

    void read(posix::fd_t wd, uint32_t watchedflags, std::unordered_multimap<posix::fd_t, EventData_t>& results)
    {
      (void)watchedflags;
      char buffer[4096] = {0};

      ssize_t n = posix::recv(wd, &buffer, sizeof(buffer), 0);
      if (n > 0)
      {
        bool do_add    = std::strncmp("add@", buffer, sizeof("add@")) == posix::success_response;
        bool do_remove = std::strncmp("remove@", buffer, sizeof("remove@")) == posix::success_response;
        if(do_add || do_remove)
        {
          char mountpoint[PATH_MAX] = {0};
          const char* majmin = std::strrchr(buffer, '/'); // find last '/'
          if(majmin == nullptr) // if invalid
            return; // exit function
          ++majmin; // get the next character


          auto name_iter = mount2fds.find(mountpoint);
          if(name_iter != mount2fds.end())
          {
            auto fd_iter = fds2mount.find(name_iter->second);
            const std::string& device = fd_iter->second.first;
            const EventFlags_t& flags = fd_iter->second.second;

            if(flags.isSet(EventFlags::MountEvent))
            {
            }
            else if(flags.isSet(EventFlags::UnmountEvent))
            {
            }
          }
        }
      }
    }
  } mountnotify;
#endif
#ifdef GLOBAL_PROCESS_EVENT_TRACKING
  struct procnotify_t // process notification (process events connector)
  {
    posix::fd_t fd;
    std::unordered_multimap<pid_t, EventFlags_t> events;

    // process flags
    static EventFlags_t from_native_flags(const uint32_t flags) noexcept
    {
      EventFlags_t data;
      data.ExecEvent = flags & proc_event::PROC_EVENT_EXEC ? 1 : 0;
      data.ExitEvent = flags & proc_event::PROC_EVENT_EXIT ? 1 : 0;
      data.ForkEvent = flags & proc_event::PROC_EVENT_FORK ? 1 : 0;
      return data;
    }

    static constexpr uint32_t to_native_flags(const EventFlags_t& flags) noexcept
    {
      return
          (flags.ExecEvent ? uint32_t(proc_event::PROC_EVENT_EXEC) : 0) | // Process called exec*()
          (flags.ExitEvent ? uint32_t(proc_event::PROC_EVENT_EXIT) : 0) | // Process exited
          (flags.ForkEvent ? uint32_t(proc_event::PROC_EVENT_FORK) : 0) ; // Process forked
    }

    procnotify_t(void) noexcept
      : fd(posix::invalid_descriptor)
    {
      fd = posix::socket(EDomain::netlink, EType::datagram, EProtocol::connector);
      flaw(fd == posix::invalid_descriptor, terminal::warning,,,
           "Unable to open a netlink socket for Process Events Connector: %s", std::strerror(errno))

      sockaddr_nl sa_nl;
      sa_nl.nl_family = PF_NETLINK;
      sa_nl.nl_groups = CN_IDX_PROC;
      sa_nl.nl_pid = getpid();

      flaw(!posix::bind(fd, (struct sockaddr *)&sa_nl, sizeof(sa_nl)), terminal::warning,,,
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

    ~procnotify_t(void) noexcept
    {
      posix::close(fd);
      fd = posix::invalid_descriptor;
    }

    posix::fd_t watch(pid_t pid, EventFlags_t flags) noexcept
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

    bool owns(posix::fd_t somefd) const noexcept { return somefd == fd; }

    void read(posix::fd_t procfd, uint32_t watchedflags, std::unordered_multimap<posix::fd_t, EventData_t>& results)
    {
      (void)watchedflags;
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
        {
          EventFlags_t flags = from_native_flags(procmsg.event.what);
          auto entries = events.equal_range(procmsg.event.event_data.id.process_pid); // get all the entries for that PID
          for(auto& pos = entries.first; pos != entries.second; ++pos)
          {
            if(pos->second.isSet(flags)) // test to see if the current process matches the triggering EventFlag
              results.emplace(pos->first,
                EventData_t(flags,
                 procmsg.event.event_data.exit.process_pid,
                 procmsg.event.event_data.exit.process_tgid,
                 procmsg.event.event_data.exit.exit_code,
                 procmsg.event.event_data.exit.exit_signal));
          }
        }
      }
    }
  } procnotify;
#endif
};

std::unordered_multimap<posix::fd_t, EventFlags_t> EventBackend::queue; // watch queue
std::unordered_multimap<posix::fd_t, EventData_t> EventBackend::results; // results from getevents()

struct platform_dependant* EventBackend::platform = nullptr;

void EventBackend::init(void) noexcept
{
  flaw(platform != nullptr, terminal::warning, posix::error(std::errc::operation_not_permitted),,
       "EventBackend::init() has been called multiple times!")
  platform = new platform_dependant;
#ifdef MOUNT_NOTIFICATIONS
  watch(platform->mountnotify.fd, EventFlags::Readable);
#endif
#ifdef GLOBAL_PROCESS_EVENT_TRACKING
  watch(platform->procnotify.fd, EventFlags::Readable);
#endif
}

void EventBackend::destroy(void) noexcept
{
  flaw(platform == nullptr, terminal::warning, posix::error(std::errc::operation_not_permitted),,
       "EventBackend::destroy() has been called multiple times!")
  delete platform;
  platform = nullptr;
}


posix::fd_t EventBackend::watch(const char* path, EventFlags_t flags) noexcept
{
  posix::fd_t fd = posix::invalid_descriptor;
  if(flags.isSet(EventFlags::FileEvent | EventFlags::DirEvent))
  {
    fd = platform->fsnotify.watch(path, flags);
    if(fd > 0 && watch(fd, EventFlags::Readable) <= 0)
    {
      platform->fsnotify.remove(fd);
      fd = posix::invalid_descriptor;
    }
  }
#ifdef MOUNT_NOTIFICATIONS
  else if(flags.isSet(EventFlags::FilesystemEvent))
  {
    fd = platform->mountnotify.watch(path, flags);
    if(fd > 0 && watch(fd, EventFlags::Readable) <= 0)
    {
      platform->mountnotify.remove(fd);
      fd = posix::invalid_descriptor;
    }
  }
#endif
  return fd;
}

posix::fd_t EventBackend::watch(int target, EventFlags_t flags) noexcept
{
  if(flags.isSet(EventFlags::ProcEvent))
#ifdef GLOBAL_PROCESS_EVENT_TRACKING
    return platform->procnotify.watch(target, flags);
#else
    return posix::invalid_descriptor;
#endif
  else if(flags.isSet(EventFlags::FDEvent))
    return platform->pollnotify.watch(target, flags);
  return posix::invalid_descriptor;
}

posix::fd_t EventBackend::lookup(const char* path) noexcept
{
  auto iterpath = platform->fsnotify.path2fds.find(path);
  if(iterpath != platform->fsnotify.path2fds.end())
    return iterpath->second;

#ifdef MOUNT_NOTIFICATIONS
  auto itermount = platform->mountnotify.mount2fds.find(path);
  if(itermount != platform->mountnotify.mount2fds.end())
    return itermount->second;
#endif

  return posix::invalid_descriptor;
}

bool EventBackend::remove(int target, EventFlags_t flags) noexcept
{
  bool result = false;
  if(flags.isSet(EventFlags::FileEvent | EventFlags::DirEvent))
  {
    flags.set(EventFlags::FDEvent);
    result |= platform->fsnotify.remove(target);
  }
#ifdef MOUNT_NOTIFICATIONS
  if(flags.isSet(EventFlags::FilesystemEvent))
  {
    flags.set(EventFlags::FDEvent);
    result |= platform->mountnotify.remove(target);
  }
#endif
  if(flags.isSet(EventFlags::FDEvent))
    result |= platform->pollnotify.remove(target);
#ifdef GLOBAL_PROCESS_EVENT_TRACKING
  if(flags.isSet(EventFlags::ProcEvent))
    result |= platform->procnotify.remove(target);
#endif
  return result;
}

bool EventBackend::getevents(int timeout) noexcept
{
  int count = platform->pollnotify.wait(timeout);
  results.clear(); // clear old results

  if(count == posix::error_response) // if error/timeout occurred
    return false; //fail

  const epoll_event* end = platform->pollnotify.output + count;
  for(epoll_event* pos = platform->pollnotify.output; pos != end; ++pos) // iterate through results
  {
    const posix::fd_t fd = pos->data.fd;
    const uint32_t events = pos->events;
    if(platform->fsnotify.owns(fd)) // if an inotify event
      platform->fsnotify.read(fd, events, results);
#ifdef MOUNT_NOTIFICATIONS
    else if(platform->mountnotify.owns(fd)) // if a process event
      platform->mountnotify.read(fd, events, results);
#endif
#ifdef GLOBAL_PROCESS_EVENT_TRACKING
    else if(platform->procnotify.owns(fd)) // if a process event
      platform->procnotify.read(fd, events, results);
#endif    
    else // normal file descriptor event
      platform->pollnotify.read(fd, events, results);
  }
  return true;
}
#elif defined(__APPLE__) || defined(BSD)

#define GLOBAL_PROCESS_EVENT_TRACKING

// BSD
#include <sys/time.h>
#include <sys/event.h>

// POSIX
#include <sys/socket.h>
#include <unistd.h>

// POSIX++
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cstring>

// STL
#include <vector>

// PDTK
#include <cxxutils/vterm.h>
#include <cxxutils/error_helpers.h>


std::unordered_multimap<posix::fd_t, EventFlags_t> EventBackend::queue; // watch queue
std::unordered_multimap<posix::fd_t, EventData_t> EventBackend::results; // results from getevents()

constexpr uint32_t to_event_filter(const EventFlags_t& flags)
{
  if(flags.Readable ||
     flags.Disconnected)
    return EVFILT_READ;
  if(flags.Writeable)
    return EVFILT_WRITE;
  if(flags.isSet(EventFlags::FileEvent | EventFlags::DirEvent))
    return EVFILT_VNODE;
  if(flags.isSet(EventFlags::ProcEvent))
    return EVFILT_PROC;
#ifdef MOUNT_NOTIFICATIONS
  if(flags.isSet(EventFlags::FilesystemEvent))
    return EVFILT_FS;
#endif
  return 0;
}

constexpr uint32_t to_native_flags(const EventFlags_t& flags)
{
  return
#ifdef GLOBAL_PROCESS_EVENT_TRACKING
// process
      (flags.ExecEvent    ? uint32_t(NOTE_EXEC    ) : 0) | // Process called exec*()
      (flags.ExitEvent    ? uint32_t(NOTE_EXIT    ) : 0) | // Process exited
      (flags.ForkEvent    ? uint32_t(NOTE_FORK    ) : 0) | // Process forked
#endif
#ifdef MOUNT_NOTIFICATIONS
// filesystem flags
      (flags.MountEvent   ? uint32_t(NOTE_MOUNTED ) : 0) | // Filesystem mounted
      (flags.UnmountEvent ? uint32_t(NOTE_UMOUNTED) : 0) | // Filesystem unmounted
#endif
// file flags
      (flags.ReadEvent    ? uint32_t(NOTE_NONE    ) : 0) | // File was read
      (flags.WriteEvent   ? uint32_t(NOTE_WRITE   ) : 0) | // File was modified (*).
      (flags.AttributeMod ? uint32_t(NOTE_ATTRIB  ) : 0) | // Metadata changed, e.g., permissions, timestamps, extended attributes, link count (since Linux 2.6.25), UID, GID, etc. (*).
      (flags.Moved        ? uint32_t(NOTE_RENAME  ) : 0) | // Watched File was moved.
      (flags.Deleted      ? uint32_t(NOTE_DELETE  ) : 0) | // Watched File was deleted.
// directory flags
      (flags.SubCreated   ? uint32_t(NOTE_WRITE   ) : 0) | // File created in watched dir.
      (flags.SubMoved     ? uint32_t(NOTE_RENAME  ) : 0) | // File moved in watched dir.
      (flags.SubDeleted   ? uint32_t(NOTE_DELETE  ) : 0) ; // File deleted in watched dir.
}


// FD flags
inline EventData_t from_kevent(const struct kevent& ev) noexcept
{
  EventData_t data;
  struct stat buf;
  data.flags.Error              = ev.flags & EV_ERROR       ? 1 : 0;

  switch (ev.filter)
  {
    case EVFILT_READ:
      data.flags.Readable       = ev.flags == 0             ? 1 : 0;
      data.flags.Disconnected   = ev.flags & EV_EOF         ? 1 : 0;
      break;

    case EVFILT_WRITE:
      data.flags.Writeable      = ev.flags == 0             ? 1 : 0;
      break;

#ifdef MOUNT_NOTIFICATIONS
    case EVFILT_FS:
      data.flags.MountEvent     = ev.flags & NOTE_MOUNTED   ? 1 : 0;
      data.flags.UnmountEvent   = ev.flags & NOTE_UMOUNTED  ? 1 : 0;
      break;
#endif

    case EVFILT_VNODE:
      data.flags.AttributeMod   = ev.flags & NOTE_ATTRIB    ? 1 : 0;
// file flags
      data.flags.WriteEvent     = ev.flags & NOTE_WRITE     ? 1 : 0;
      data.flags.Moved          = ev.flags & NOTE_RENAME    ? 1 : 0;
      data.flags.Deleted        = ev.flags & NOTE_DELETE    ? 1 : 0;
/*
      ::fstat(ev.indent, &buf);
      if(buf.st_mode & S_IFDIR)
      {
        // directory flags
        data.flags.SubCreated   = ev.flags & NOTE_WRITE     ? 1 : 0;
        data.flags.SubMoved     = ev.flags & NOTE_RENAME    ? 1 : 0;
        data.flags.SubDeleted   = ev.flags & NOTE_DELETE    ? 1 : 0;
      }
      else
      {
        // file flags
        data.flags.WriteEvent   = ev.flags & NOTE_WRITE     ? 1 : 0;
        data.flags.Moved        = ev.flags & NOTE_RENAME    ? 1 : 0;
        data.flags.Deleted      = ev.flags & NOTE_DELETE    ? 1 : 0;
      }
*/
      break;

#ifdef GLOBAL_PROCESS_EVENT_TRACKING
    case EVFILT_PROC:
    // process flags
      data.flags.ExecEvent      = ev.flags & NOTE_EXEC      ? 1 : 0;
      data.flags.ExitEvent      = ev.flags & NOTE_EXIT      ? 1 : 0;
      data.flags.ForkEvent      = ev.flags & NOTE_FORK      ? 1 : 0;
      break;
#endif
  }


  return data;
}


struct platform_dependant
{
  posix::fd_t kq;
  std::vector<struct kevent> kinput;   // events we want to monitor
  std::vector<struct kevent> koutput;   // events that were triggered

  platform_dependant(void)
  {
    kq = posix::ignore_interruption(::kqueue);
    flaw(kq == posix::error_response, terminal::critical, std::exit(errno),,
         "Unable to create a new kqueue: %s", std::strerror(errno))
    kinput .reserve(1024);
    koutput.reserve(1024);
  }

  ~platform_dependant(void)
  {
    posix::close(kq);
  }
};

struct platform_dependant* EventBackend::platform = nullptr;

void EventBackend::init(void) noexcept
{
  flaw(platform != nullptr, terminal::warning, posix::error(std::errc::operation_not_permitted),,
       "EventBackend::init() has been called multiple times!")
  platform = new platform_dependant;
}

void EventBackend::destroy(void) noexcept
{
  flaw(platform == nullptr, terminal::warning, posix::error(std::errc::operation_not_permitted),,
       "EventBackend::destroy() has been called multiple times!")
  delete platform;
  platform = nullptr;
}


posix::fd_t EventBackend::watch(const char* path, EventFlags_t flags) noexcept
{
  return posix::invalid_descriptor;
}

posix::fd_t EventBackend::watch(int target, EventFlags_t flags) noexcept
{
  struct kevent ev;
  EV_SET(&ev, target, to_event_filter(flags), EV_ADD, to_native_flags(flags), 0, nullptr);
  platform->kinput.push_back(ev);
  platform->koutput.resize(platform->kinput.size());
  queue.emplace(target, flags);
  return target;
}

bool EventBackend::remove(int target, EventFlags_t flags) noexcept
{
  return false;
}

bool EventBackend::getevents(int timeout) noexcept
{
  EventData_t data;
  timespec tout;
  tout.tv_sec = timeout / 1000;
  tout.tv_nsec = (timeout % 1000) * 1000;

  int count = 0;
  count = kevent(platform->kq,
             platform->kinput.data(), platform->kinput.size(),
             platform->koutput.data(), platform->koutput.size(),
             &tout);

  if(count <= 0)
    return false;

  struct kevent* end = platform->koutput.data() + count;

  for(struct kevent* pos = platform->koutput.data(); pos != end; ++pos) // iterate through results
  {
    data = from_kevent(*pos);

    results.emplace(posix::fd_t(pos->ident), data);
  }
  return true;
}

#elif defined(__sun) && defined(__SVR4) // Solaris

#pragma message Not implemented, yet!
#pragma message See: http://docs.oracle.com/cd/E19253-01/816-5168/port-get-3c/index.html

// Solaris
#include <port.h>

// POSIX
#include <sys/socket.h>
#include <unistd.h>

// POSIX++
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cstring>

// STL
#include <vector>

// PDTK
#include <cxxutils/vterm.h>
#include <cxxutils/error_helpers.h>


struct platform_dependant
{
  posix::fd_t port;
  std::vector<port_event_t> pinput;   // events we want to monitor
  std::vector<port_event_t> poutput;  // events that were triggered

  platform_dependant(void)
  {
    port = ::port_create();
    flaw(port == posix::error_response, terminal::critical, std::exit(errno),,
         "Unable to create a new kqueue: %s", std::strerror(errno))
   pinput .reserve(1024);
   poutput.reserve(1024);
  }

  ~platform_dependant(void)
  {
    posix::close(port);
  }
};

struct platform_dependant* EventBackend::platform = nullptr;

void EventBackend::init(void) noexcept
{
  flaw(platform != nullptr, terminal::warning, posix::error(std::errc::operation_not_permitted),,
       "EventBackend::init() has been called multiple times!")
  platform = new platform_dependant;
}

void EventBackend::destroy(void) noexcept
{
  flaw(platform == nullptr, terminal::warning, posix::error(std::errc::operation_not_permitted),,
       "EventBackend::destroy() has been called multiple times!")
  delete platform;
  platform = nullptr;
}


posix::fd_t EventBackend::watch(const char* path, EventFlags_t flags) noexcept
{
  return posix::invalid_descriptor;
}

posix::fd_t EventBackend::watch(int target, EventFlags_t flags) noexcept
{
  port_event_t pev;

  platform->pinput.push_back(pev);
  queue.emplace(target, flags);
  return target;
}

bool EventBackend::remove(int target, EventFlags_t flags) noexcept
{
  return false;
}

bool EventBackend::getevents(int timeout) noexcept
{
  uint32_t flags;
  uint_t count = 0;
  timespec tout;
  tout.tv_sec = timeout / 1000;
  tout.tv_nsec = (timeout % 1000) * 1000;

  if(::port_getn(platform->port,
                 &platform->pinput.data(), platform->pinput.size(),
                 platform->poutput, MAX_EVENTS, &count
                 &tout) == posix::error_response)
    return false;

  port_event_t* end = platform->poutput + count;

  for(port_event_t* pos = platform->poutput; pos != end; ++pos) // iterate through results
  {
    //flags = from_kevent(*pos);

    results.emplace(posix::fd_t(pos->ident), flags);
  }
  return true;
}


#elif defined(__unix__)

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif
