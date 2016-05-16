#include "eventbackend.h"

#if defined(__linux__)

// Linux
#include <sys/epoll.h>

// STL
#include <cassert>

// POSIX
#include <unistd.h>

#define MAX_EVENTS 32000

EventFlags_t EventFlags_t::from_native(const uint32_t flags)
{
  EventFlags_t rval;
  rval.Error       = flags & EPOLLERR ? 1 : 0;
  rval.Disconnect  = flags & EPOLLHUP ? 1 : 0;
  rval.Read        = flags & EPOLLIN  ? 1 : 0;
  rval.Write       = flags & EPOLLOUT ? 1 : 0;
  rval.EdgeTrigger = flags & EPOLLET  ? 1 : 0;
//  rval.Persistant  = flags & NATIVE_FLAG ? 1 : 0;
//  rval.Timeout     = flags & NATIVE_FLAG ? 1 : 0;
//  rval.Signal      = flags & NATIVE_FLAG ? 1 : 0;
  return rval;
}

uint32_t EventFlags_t::to_native(const EventFlags_t flags)
{
  return
      (flags.Error       ? (uint32_t)EPOLLERR : 0) |
      (flags.Disconnect  ? (uint32_t)EPOLLHUP : 0) |
      (flags.Read        ? (uint32_t)EPOLLIN  : 0) |
      (flags.Write       ? (uint32_t)EPOLLOUT : 0) |
      (flags.EdgeTrigger ? (uint32_t)EPOLLET  : 0);
//      (flags.Persistant  ? NATIVE_FLAG : 0) |
//      (flags.Timeout     ? NATIVE_FLAG : 0) |
//      (flags.Signal      ? NATIVE_FLAG : 0) |
}

struct platform_dependant
{
  struct epoll_event output[MAX_EVENTS];
  int num_events;
};

EventBackend::EventBackend(void)
//  : features({1,1,1,1})
{
  m_data = new platform_dependant;
  m_pollfd = epoll_create(MAX_EVENTS);
  assert(m_pollfd != posix::error_response);
  m_results.reserve(MAX_EVENTS);
}

EventBackend::~EventBackend(void)
{
  delete m_data;
  m_data = nullptr;

  ::close(m_pollfd);
  m_pollfd = posix::error_response;
}

bool EventBackend::watch(posix::fd_t fd, EventFlags_t events)
{
  struct epoll_event native_event;
  native_event.data.fd = fd;
  native_event.events = EventFlags_t::to_native(events); // be sure to convert to native events
  auto iter = m_queue.find(fd); // search queue for FD
  if(iter != m_queue.end()) // entry exists for FD
  {
    if(epoll_ctl(m_pollfd, EPOLL_CTL_MOD, fd, &native_event) == posix::success_response || // try to modify FD first
       (errno == ENOENT && epoll_ctl(m_pollfd, EPOLL_CTL_ADD, fd, &native_event) == posix::success_response)) // error: FD not added, so try adding
    {
      iter->second = events; // set value of existing entry
      posix::success();
    }
  }
  else if(epoll_ctl(m_pollfd, EPOLL_CTL_ADD, fd, &native_event) == posix::success_response || // try adding FD first
          (errno == EEXIST && epoll_ctl(m_pollfd, EPOLL_CTL_MOD, fd, &native_event) == posix::success_response)) // error: FD already added, so try modifying
  {
    m_queue.emplace(fd, events); // create a new entry
    posix::success();
  }

  return errno == posix::success_response;
}

bool EventBackend::remove(posix::fd_t fd)
{
  struct epoll_event event;
  auto iter = m_queue.find(fd); // search queue for FD
  if(iter == m_queue.end() && // entry exists for FD
     epoll_ctl(m_pollfd, EPOLL_CTL_DEL, fd, &event) == posix::success_response) // try to delete entry
    m_queue.erase(iter); // remove entry for FD
  return errno == posix::success_response;
}

bool EventBackend::invoke(int timeout)
{
  m_data->num_events = epoll_wait(m_pollfd, m_data->output, MAX_EVENTS, timeout); // wait for new results
  m_results.clear(); // clear old results

  if(m_data->num_events == posix::error_response) // if error occurred
    return false; //fail

  posix::fd_t fd;
  const struct epoll_event* end = m_data->output + m_data->num_events;
  for(struct epoll_event* pos = m_data->output; pos != end; ++pos) // iterate through results
    m_results.emplace(fd = pos->data.fd, EventFlags_t::from_native(pos->events)); // save result (in non-native format)
  return true;
}
#elif defined(__unix__)

#error no code yet for your operating system. :(

#endif
