#include "eventbackend.h"

#define MAX_EVENTS 2048

//#if 0
#if defined(__linux__)

// Linux
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <linux/connector.h>
#include <linux/netlink.h>
#include <linux/cn_proc.h>

// STL
#include <algorithm>
#include <set>

// POSIX
#include <sys/socket.h>
#include <unistd.h>

// POSIX++
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cstring>

// PDTK
#include <cxxutils/colors.h>
#include <cxxutils/error_helpers.h>


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

constexpr uint32_t to_native_fileflags(const EventFlags_t& flags)
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

#ifdef ENABLE_PROCESS_EVENT_TRACKING
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
#endif

struct platform_dependant
{
  struct pollnotify_t // poll notification (epoll)
  {
    posix::fd_t fd;
    std::unordered_multimap<posix::fd_t, EventFlags_t>& fds;
    struct epoll_event output[MAX_EVENTS];

    pollnotify_t(void) noexcept
      : fd(posix::invalid_descriptor), fds(EventBackend::queue)
    {
      fd = epoll_create(MAX_EVENTS);
      flaw(fd == posix::invalid_descriptor, posix::critical, std::exit(errno),,
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
      native_event.events = to_native_fdflags(flags); // be sure to convert to native events
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

  } pollnotify;

  struct fsnotify_t // file notification (inotify)
  {
    posix::fd_t fd;
    std::set<posix::fd_t> fds;

    fsnotify_t(void) noexcept
      : fd(posix::invalid_descriptor)
    {
      fd = inotify_init();
      flaw(fd == posix::invalid_descriptor, posix::severe,,,
           "Unable to create an instance of inotify!: %s", std::strerror(errno))
    }

    ~fsnotify_t(void) noexcept
    {
      posix::close(fd);
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

#ifdef ENABLE_PROCESS_EVENT_TRACKING
  struct procnotify_t // process notification (process events connector)
  {
    posix::fd_t fd;
    std::unordered_multimap<pid_t, EventFlags_t> events;

    procnotify_t(void) noexcept
      : fd(posix::invalid_descriptor)
    {
      fd = ::socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
      flaw(fd == posix::invalid_descriptor, posix::warning,,,
           "Unable to open a netlink socket for Process Events Connector: %s", std::strerror(errno))

      sockaddr_nl sa_nl;
      sa_nl.nl_family = PF_NETLINK;
      sa_nl.nl_groups = CN_IDX_PROC;
      sa_nl.nl_pid = getpid();
      int binderr = ::bind(fd, (struct sockaddr *)&sa_nl, sizeof(sa_nl));
      flaw(binderr == posix::error_response, posix::warning,,,
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

      flaw(::send(fd, &procconn, sizeof(procconn), 0) == posix::error_response, posix::warning,,,
           "Failed to enable Process Events Connector notifications: %s", std::strerror(errno))
    }

    ~procnotify_t(void) noexcept
    {
      posix::close(fd);
      fd = posix::error_response;
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
  } procnotify;
#endif
};

std::unordered_multimap<posix::fd_t, EventFlags_t> EventBackend::queue; // watch queue
std::unordered_multimap<posix::fd_t, EventData_t> EventBackend::results; // results from getevents()

struct platform_dependant* EventBackend::platform = nullptr;

void EventBackend::init(void) noexcept
{
  flaw(platform != nullptr, posix::warning, posix::error(std::errc::operation_not_permitted),,
       "EventBackend::init() has been called multiple times!")
  platform = new platform_dependant;
#ifdef ENABLE_PROCESS_EVENT_TRACKING
  watch(platform->procnotify.fd, EventFlags::Readable);
#endif
}

void EventBackend::destroy(void) noexcept
{
  flaw(platform == nullptr, posix::warning, posix::error(std::errc::operation_not_permitted),,
       "EventBackend::destroy() has been called multiple times!")
  delete platform;
  platform = nullptr;
}


posix::fd_t EventBackend::watch(const char* path, EventFlags_t flags) noexcept
{
  posix::fd_t fd = platform->fsnotify.watch(path, flags);
  if(fd > 0 && watch(fd, EventFlags::Readable) <= 0)
  {
    platform->fsnotify.remove(fd);
    fd = posix::invalid_descriptor;
  }
  return fd;
}

posix::fd_t EventBackend::watch(int target, EventFlags_t flags) noexcept
{
  if(flags | EventFlags::ProcEvent)
#ifdef ENABLE_PROCESS_EVENT_TRACKING
    return platform->procnotify.watch(target, flags);
#else
    return posix::invalid_descriptor;
#endif
  else if(flags | EventFlags::FDEvent)
    return platform->pollnotify.watch(target, flags);
  return posix::invalid_descriptor;
}


bool EventBackend::remove(int target, EventFlags_t flags) noexcept
{
#ifdef ENABLE_PROCESS_EVENT_TRACKING
  if(flags | EventFlags::ProcEvent)
    return platform->procnotify.remove(target) && platform->pollnotify.remove(target);
#else
  if(flags | EventFlags::FileEvent | EventFlags::DirEvent)
    return (platform->fsnotify.remove(target) && platform->pollnotify.remove(target)) ||
           platform->pollnotify.remove(target);
#endif
  else if(flags | EventFlags::FDEvent)
    return platform->pollnotify.remove(target);
  return false;
}

#define INOTIFY_EVENT_SIZE   (sizeof(inotify_event) + NAME_MAX + 1)

bool EventBackend::getevents(int timeout) noexcept
{
  int count = platform->pollnotify.wait(timeout);
  results.clear(); // clear old results

  if(count == posix::error_response) // if error/timeout occurred
    return false; //fail

  uint8_t inotifiy_buffer_data[INOTIFY_EVENT_SIZE * 16]; // queue has a minimum of size of 16 inotify events

  const epoll_event* end = platform->pollnotify.output + count;
  for(epoll_event* pos = platform->pollnotify.output; pos != end; ++pos) // iterate through results
  {
#ifdef ENABLE_PROCESS_EVENT_TRACKING
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

      pollfd fds = { pos->data.fd, POLLIN, 0 };
      while(posix::ignore_interruption(::poll, &fds, nfds_t(1), 0) > 0) // while there are messages
      {
        if(posix::ignore_interruption(::recv, platform->procnotify.fd, reinterpret_cast<void*>(&procnote), sizeof(procnote), 0) > 0) // read process event message
        {
          EventFlags_t flags = from_native_procflags(procnote.event.what);
          auto entries = platform->procnotify.events.equal_range(procnote.event.event_data.id.process_pid); // get all the entries for that PID
          for_each(entries.first, entries.second, // for each matching PID entry
            [&procnote, flags](const std::pair<pid_t, EventFlags_t>& pair)
            {
              if(pair.second.isSet(flags)) // test to see if the current process matches the triggering EventFlag
                results.emplace(pair.first,
                  EventData_t(flags,
                   procnote.event.event_data.exit.process_pid,
                   procnote.event.event_data.exit.process_tgid,
                   procnote.event.event_data.exit.exit_code,
                   procnote.event.event_data.exit.exit_signal));
            });
        }
      }
    }
    else
#endif
    if(platform->fsnotify.fds.find(pos->data.fd) != platform->fsnotify.fds.end()) // if an inotify event
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
//#elif 1
#elif defined(__APPLE__) || defined(BSD)

#define ENABLE_PROCESS_EVENT_TRACKING

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

// PDTK
#include <cxxutils/colors.h>
#include <cxxutils/error_helpers.h>


std::unordered_multimap<posix::fd_t, EventFlags_t> EventBackend::queue; // watch queue
std::unordered_multimap<posix::fd_t, EventData_t> EventBackend::results; // results from getevents()

constexpr uint32_t to_event_filter(const EventFlags_t& flags)
{
  if(flags.Readable ||
     flags.Disconnected ||
     flags.ReadEvent)
    return EVFILT_READ;
  if(flags.Writeable ||
     flags.WriteEvent)
    return EVFILT_WRITE;
  if(flags | EventFlags::FileEvent)
    return EVFILT_VNODE;
  if(flags | EventFlags::ProcEvent)
    return EVFILT_PROC;
  return 0;
}

constexpr uint32_t to_native_flags(const EventFlags_t& flags)
{
  return
#ifdef ENABLE_PROCESS_EVENT_TRACKING
// process
      (flags.ExecEvent    ? uint32_t(NOTE_EXEC  ) : 0) | // Process called exec*()
      (flags.ExitEvent    ? uint32_t(NOTE_EXIT  ) : 0) | // Process exited
      (flags.ForkEvent    ? uint32_t(NOTE_FORK  ) : 0) | // Process forked
#endif
// file flags
      (flags.WriteEvent   ? uint32_t(NOTE_WRITE ) : 0) | // File was modified (*).
      (flags.AttributeMod ? uint32_t(NOTE_ATTRIB) : 0) | // Metadata changed, e.g., permissions, timestamps, extended attributes, link count (since Linux 2.6.25), UID, GID, etc. (*).
      (flags.Moved        ? uint32_t(NOTE_RENAME) : 0) | // Watched File was moved.
      (flags.Deleted      ? uint32_t(NOTE_DELETE) : 0) | // Watched File was deleted.
// directory flags
      (flags.SubCreated   ? uint32_t(NOTE_WRITE ) : 0) | // File created in watched dir.
      (flags.SubMoved     ? uint32_t(NOTE_RENAME) : 0) | // File moved in watched dir.
      (flags.SubDeleted   ? uint32_t(NOTE_DELETE) : 0) ; // File deleted in watched dir.
}


// FD flags
inline EventData_t from_native_fdflags(const uint32_t flags) noexcept
{
  EventData_t data;
  data.flags.Error        = flags & EV_ERROR     ? 1 : 0;
  data.flags.Disconnected = flags & EV_EOF       ? 1 : 0;
  return data;
}

// file/directory flags
inline EventData_t from_native_fileflags(const uint32_t flags) noexcept
{
  EventData_t data;
  data.flags.WriteEvent   = flags & NOTE_WRITE  ? 1 : 0;
  data.flags.AttributeMod = flags & NOTE_ATTRIB ? 1 : 0;
  data.flags.Moved        = flags & NOTE_RENAME ? 1 : 0;
  data.flags.Deleted      = flags & NOTE_DELETE ? 1 : 0;
  data.flags.SubCreated   = flags & NOTE_WRITE  ? 1 : 0;
  data.flags.SubMoved     = flags & NOTE_RENAME ? 1 : 0;
  data.flags.SubDeleted   = flags & NOTE_DELETE ? 1 : 0;
  return data;
}

// process flags
inline EventFlags_t from_native_procflags(const uint32_t flags) noexcept
{
  EventFlags_t rval;
  rval.ExecEvent = flags & NOTE_EXEC ? 1 : 0;
  rval.ExitEvent = flags & NOTE_EXIT ? 1 : 0;
  rval.ForkEvent = flags & NOTE_FORK ? 1 : 0;
  return rval;
}


struct platform_dependant
{
  posix::fd_t kq;
  std::vector<kevent> kinput;   // events we want to monitor
  kevent koutput[MAX_EVENTS];   // events that were triggered

  platform_dependant(void)
  {
    kq = posix::ignore_interruption(::kqueue);
    flaw(kq == posix::error_response, posix::critical, std::exit(errno),,
         "Unable to create a new kqueue: %s", std::strerror(errno))
  }

  ~platform_dependant(void)
  {
    posix::close(kq);
  }
};

struct platform_dependant* EventBackend::platform = nullptr;

void EventBackend::init(void) noexcept
{
  flaw(platform != nullptr, posix::warning, posix::error(std::errc::operation_not_permitted),,
       "EventBackend::init() has been called multiple times!")
  platform = new platform_dependant;
}

void EventBackend::destroy(void) noexcept
{
  flaw(platform == nullptr, posix::warning, posix::error(std::errc::operation_not_permitted),,
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
  kevent ev;
  EV_SET(&ev, target, to_event_filter(flags), EV_ADD, to_native_flags(flags), 0, nullptr);
  platform->kinput.emplace(ev);
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
  timespec tout;
  tout.tv_sec = timeout / 1000;
  tout.tv_nsec = (timeout % 1000) * 1000;

  int count = 0;
  count = kevent(platform->kq,
             &platform->kinput.data(), platform->kinput.size(),
             platform->koutput, MAX_EVENTS,
             &tout);

  if(count <= 0)
    return false;

  kevent* end = platform->koutput + count;

  for(kevent* pos = platform->koutput; pos != end; ++pos) // iterate through results
  {
    flags = 0;
    switch(pos->filter)
    {
      case EVFILT_READ:
      case EVFILT_WRITE:
        flags = from_native_fdflags(pos->filter);
        flags = from_native_fileflags(pos->filter);
        break;

      case EVFILT_VNODE:
        flags = from_native_fileflags(pos->flags);
        break;

      case EVFILT_PROC:
        flags = from_native_procflags(pos->flags);
        break;

      default:
        return false;
    }
    results.emplace(posix::fd_t(pos->ident), flags);
  }
  return true;
}

#elif defined(__unix__)

#error no code yet for your operating system. :(

#else
#error Unsupported platform! >:(
#endif
