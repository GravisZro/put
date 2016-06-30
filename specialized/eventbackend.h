#ifndef EVENTBACKEND_H
#define EVENTBACKEND_H

// POSIX
#include <poll.h>
//#include <time.h>
#include <sys/types.h>

// STL
#include <unordered_map>
#include <vector>

// PDTK
#include <cxxutils/posix_helpers.h>

enum EventFlags : uint32_t
{
  Invalid     = 0x0000,
  Error       = 0x0001, // FD encountered an error (output only)
  Disconnect  = 0x0002, // FD has disconnected (output only)
  Read        = 0x0004, // FD is readable
  Write       = 0x0008, // FD is writeable
  EdgeTrigger = 0x0010, // FD will be edge-triggered (input only)
//  Persistant  = 0x0020, // When a persistent event with a timeout becomes activated, its timeout is reset to 0.
//  Timeout     = 0x0040, // Indicates that a timeout has occurred.
//  Signal      = 0x0080, // Wait for a POSIX signal to be raised
};

struct EventFlags_t
{
  uint32_t Error        : 1;
  uint32_t Disconnect   : 1;
  uint32_t Read         : 1;
  uint32_t Write        : 1;
  uint32_t EdgeTrigger  : 1;
//  uint32_t Persistant   : 1;
//  uint32_t Timeout      : 1;
//  uint32_t Signal       : 1;

  EventFlags_t(uint32_t flags = 0) noexcept { *reinterpret_cast<uint32_t*>(this) = flags; }
  operator uint32_t(void) const noexcept { return *reinterpret_cast<const uint32_t*>(this); }

  static EventFlags_t from_native(uint32_t flags) noexcept;
  static uint32_t to_native(EventFlags_t flags) noexcept; // translates flags to the native platform flags
};
static_assert(sizeof(EventFlags_t) == sizeof(uint32_t), "EventFlags_t: bad size");

struct pollfd_t
{
  posix::fd_t fd;
  EventFlags_t events;
  EventFlags_t revents;

  pollfd_t(posix::fd_t _fd = 0, EventFlags_t _events = 0, EventFlags_t _revents = 0) noexcept
    : fd(_fd), events(_events), revents(_revents) { }
};


class EventBackend
{
public:
/*
  struct features_t
  {
    uint8_t thread_specific     : 1; // Requires reinitialization when a new thread is created
    uint8_t edge_trigged        : 1; // Event method that allows edge-triggered events with EV_ET.
    uint8_t O1_efficient        : 1; // Event method where having one event triggered among many is [approximately] an O(1) operation. (excludes "select" and "poll")
    uint8_t allows_non_sockets  : 1; // Event method that allows file descriptors as well as sockets.
  };
*/

  EventBackend(void) noexcept;
 ~EventBackend(void) noexcept;

  /* Enable reading/writing on a given fd or signal.
      - 'events' = the events that we're trying to enable: one or more of EV_READ, EV_WRITE, EV_SIGNAL, and EV_ET.
   */
  bool watch(posix::fd_t fd, EventFlags_t events) noexcept; // sets which events to montior in the watch queue

  bool remove(posix::fd_t fd) noexcept; // remove from watch queue

  /* Function to implement the core of an event loop.  It must see which
     added events are ready, and cause event_active to be called for each
     active event (usually via event_io_active or such). */
  bool invoke(int timeout = -1) noexcept;

  const std::unordered_map<posix::fd_t, EventFlags_t>& results(void) const noexcept { return m_results; }

  const std::unordered_map<posix::fd_t, EventFlags_t>& queue(void) const noexcept { return m_queue; }

  // Bit-array of features that this backend has.
//  const struct features_t features;

private:
  posix::fd_t m_pollfd;
  std::unordered_map<posix::fd_t, EventFlags_t> m_queue; // watch queue
  std::unordered_map<posix::fd_t, EventFlags_t> m_results;
  struct platform_dependant* m_data;
};

#endif // EVENTBACKEND_H
