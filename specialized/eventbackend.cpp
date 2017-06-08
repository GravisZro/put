#include "eventbackend.h"

#if defined(__linux__)

// Linux
#include <sys/inotify.h>
#include <sys/epoll.h>

// C++
#include <cassert>
#include <climits>
#include <unordered_map>

// POSIX
#include <unistd.h>

#define MAX_EVENTS 32000


inline EventFlags_t from_native_fdflags(const uint32_t flags) noexcept
{
  EventFlags_t rval;
  rval.Error        = flags & EPOLLERR ? 1 : 0;
  rval.Disconnected = flags & EPOLLHUP ? 1 : 0;
  rval.Readable     = flags & EPOLLIN  ? 1 : 0;
  rval.Writeable    = flags & EPOLLOUT ? 1 : 0;
  rval.EdgeTrigger  = flags & EPOLLET  ? 1 : 0;
  return rval;
}

inline EventFlags_t from_native_fileflags(const uint32_t flags) noexcept
{
  EventFlags_t rval;
  rval.ReadEvent    = flags & IN_ACCESS    ? 1 : 0;
  rval.WriteEvent   = flags & IN_MODIFY    ? 1 : 0;
  rval.AttributeMod = flags & IN_ATTRIB    ? 1 : 0;
  rval.Moved        = flags & IN_MOVE_SELF ? 1 : 0;
  return rval;
}

constexpr uint32_t to_native_fileflags(const EventFlags_t& flags)
{
  return
      (flags.ReadEvent    ? uint32_t(IN_ACCESS   ) : 0) | // File was accessed (read) (*).
      (flags.WriteEvent   ? uint32_t(IN_MODIFY   ) : 0) | // File was modified (*).
      (flags.AttributeMod ? uint32_t(IN_ATTRIB   ) : 0) | // Metadata changed, e.g., permissions, timestamps, extended attributes, link count (since Linux 2.6.25), UID, GID, etc. (*).
      (flags.Moved        ? uint32_t(IN_MOVE_SELF) : 0) ; // Watched file/directory was itself moved.
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

struct platform_dependant
{
  posix::fd_t m_pollfd;
  struct epoll_event output[MAX_EVENTS];
  int num_events;
  struct
  {
    posix::fd_t handle;
    std::unordered_map<std::string, posix::fd_t> map;
    std::set<posix::fd_t> fds;
  } fsnotify;

  platform_dependant(void)
  {
    fsnotify.handle = inotify_init();
    m_pollfd = epoll_create(MAX_EVENTS);
    assert(m_pollfd != posix::error_response);
  }

  ~platform_dependant(void)
  {
    ::close(m_pollfd);
    m_pollfd = posix::error_response;
    ::close(fsnotify.handle);
  }
};

std::multimap<posix::fd_t, EventFlags_t> EventBackend::queue; // watch queue
std::multimap<posix::fd_t, EventFlags_t> EventBackend::results;

struct platform_dependant* EventBackend::platform = nullptr;

void EventBackend::init(void) noexcept
{
  assert(platform == nullptr);
  platform = new platform_dependant;
  //results.reserve(MAX_EVENTS);
}

void EventBackend::destroy(void) noexcept
{
  assert(platform != nullptr);
  delete platform;
  platform = nullptr;
}

posix::fd_t EventBackend::watch(const char* path, EventFlags_t events) noexcept
{
  posix::fd_t fd = inotify_add_watch(platform->fsnotify.handle, path, to_native_fileflags(events));
  if(fd < 0)
    return fd;

  platform->fsnotify.fds.emplace(fd);
  if(!watch(fd, EventFlags::Readable))
  {
    platform->fsnotify.fds.erase(fd);
    assert(errno < 0);
    return errno;
  }

  return fd;
}

bool EventBackend::watch(posix::fd_t fd, EventFlags_t events) noexcept
{
  struct epoll_event native_event;
  native_event.data.fd = fd;
  native_event.events = to_native_fdflags(events); // be sure to convert to native events
  auto iter = queue.find(fd); // search queue for FD
  if(iter != queue.end()) // entry exists for FD
  {
    if(epoll_ctl(platform->m_pollfd, EPOLL_CTL_MOD, fd, &native_event) == posix::success_response || // try to modify FD first
       (errno == ENOENT && epoll_ctl(platform->m_pollfd, EPOLL_CTL_ADD, fd, &native_event) == posix::success_response)) // error: FD not added, so try adding
    {
      iter->second = events; // set value of existing entry
      posix::success();
    }
  }
  else if(epoll_ctl(platform->m_pollfd, EPOLL_CTL_ADD, fd, &native_event) == posix::success_response || // try adding FD first
          (errno == EEXIST && epoll_ctl(platform->m_pollfd, EPOLL_CTL_MOD, fd, &native_event) == posix::success_response)) // error: FD already added, so try modifying
  {
    queue.emplace(fd, events); // create a new entry
    posix::success();
  }

  return errno == posix::success_response;
}

bool EventBackend::remove(posix::fd_t fd) noexcept
{
  struct epoll_event event;
  auto iter = queue.find(fd); // search queue for FD
  if(iter == queue.end() && // entry exists for FD
     epoll_ctl(platform->m_pollfd, EPOLL_CTL_DEL, fd, &event) == posix::success_response) // try to delete entry
    queue.erase(iter); // remove entry for FD
  return errno == posix::success_response;
}

enum {
  Read = 0,
  Write = 1,
};

#define INOTIFY_EVENT_SIZE   (sizeof(struct inotify_event) + NAME_MAX + 1)
#include <iostream>
bool EventBackend::getevents(int timeout) noexcept
{
  platform->num_events = epoll_wait(platform->m_pollfd, platform->output, MAX_EVENTS, timeout); // wait for new results
  results.clear(); // clear old results

  if(platform->num_events == posix::error_response) // if error/timeout occurred
    return false; //fail

  union {
    uint8_t inotifiy_buffer_data[INOTIFY_EVENT_SIZE];
    struct inotify_event inotifiy_buffer;
  };

  const struct epoll_event* end = platform->output + platform->num_events;
  for(struct epoll_event* pos = platform->output; pos != end; ++pos) // iterate through results
  {
    if(platform->fsnotify.fds.count(pos->data.fd)) // if a filesystem event
    {
      ssize_t bcount = posix::read(pos->data.fd, &inotifiy_buffer, sizeof(INOTIFY_EVENT_SIZE));
      std::cout << "inotify size: " << (sizeof(struct inotify_event) + inotifiy_buffer.len) << std::endl;
      std::cout << "inotify bytes: " << bcount << std::endl;
      results.emplace(posix::fd_t(pos->data.fd), from_native_fileflags(inotifiy_buffer.mask)); // save result (in non-native format)

/*
      uint8_t* inend = inotifiy_buffer_data + posix::read(pos->data.fd, &inotifiy_buffer, sizeof(INOTIFY_EVENT_SIZE));
      union {
        uint8_t* inpos;
        struct inotify_event* incur;
      };
      for(inpos = inotifiy_buffer_data; inpos < inend; inpos += sizeof(struct inotify_event) + incur->len)
        m_results.emplace(pos->data.fd, from_native_fileflags(inotifiy_buffer.mask)); // save result (in non-native format)
*/
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
