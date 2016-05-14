#include "eventbackend.h"

#if defined(__linux__)

// Linux
#include <sys/epoll.h>

// STL
#include <cassert>

// POSIX
#include <unistd.h>

#define MAX_EVENTS 32000

#define NATIVE_FLAG 1

void EventFlags_t::from_native(uint32_t flags)
{
//  Timeout     = flags & NATIVE_FLAG ? 1 : 0;
  Read        = flags & EPOLLIN ? 1 : 0;
  Write       = flags & EPOLLOUT ? 1 : 0;
//  Signal      = flags & NATIVE_FLAG ? 1 : 0;
//  Persistant  = flags & NATIVE_FLAG ? 1 : 0;
  EdgeTrigger = flags & EPOLLET ? 1 : 0;
}

uint32_t EventFlags_t::to_native(void)
{
  return
//      (Timeout     ? NATIVE_FLAG : 0) |
      (Read        ? (uint32_t)EPOLLIN  : 0) |
      (Write       ? (uint32_t)EPOLLOUT : 0) |
//      (Signal      ? NATIVE_FLAG : 0) |
//      (Persistant  ? NATIVE_FLAG : 0) |
      (EdgeTrigger ? (uint32_t)EPOLLET  : 0);
}

struct platform_dependant
{
  struct epoll_event output[MAX_EVENTS];
  int num_events;
};

EventBackend::EventBackend(void)
  : features({1,1,1,1})
{
  m_data = new platform_dependant;
  m_pollfd = epoll_create(MAX_EVENTS);
  assert(m_pollfd != posix::error_response);
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
  struct epoll_event event;
  event.data.fd = fd;
  event.events = events.to_native();
  auto iter = m_queue.find(fd); // search queue for FD
  if(iter != m_queue.end()) // entry exists for FD
  {
    if(epoll_ctl(m_pollfd, EPOLL_CTL_MOD, fd, &event) == posix::success_response || // try to modify FD first
       (errno == ENOENT && epoll_ctl(m_pollfd, EPOLL_CTL_ADD, fd, &event) == posix::success_response)) // error: FD not added, so try adding
      iter->second.events = events; // set value of existing entry
  }
  else if(epoll_ctl(m_pollfd, EPOLL_CTL_ADD, fd, &event) == posix::success_response) // try adding
    m_queue.emplace(fd, pollfd_t(fd, events, 0)); // create a new entry

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
  m_data->num_events = epoll_wait(m_pollfd, m_data->output, MAX_EVENTS, timeout);
  if(m_data->num_events >= 0)
  {
    m_results.resize(m_data->num_events);
    struct epoll_event* output = m_data->output;
    for(auto& pair : m_results)
    {
      pair.first = output->data.fd;
      pair.second.from_native(output->events);
      ++output;
    }
  }

  return m_data->num_events != posix::error_response;
}
#elif defined(__unix__)

#error no code yet for your operating system. :(

#endif
