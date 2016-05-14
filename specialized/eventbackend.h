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
  Timeout     = 0x0001, // Indicates that a timeout has occurred.
  Read        = 0x0002, // Wait for a socket/FD to become readable
  Write       = 0x0004, // Wait for a socket/FD to become writeable
  Signal      = 0x0008, // Wait for a POSIX signal to be raised
  Presistant  = 0x0010, // When a persistent event with a timeout becomes activated, its timeout is reset to 0.
  EdgeTrigger = 0x0020, // Select edge-triggered behavior, if supported by the backend.
};

struct EventFlags_t
{
  uint32_t Timeout     : 1;
  uint32_t Read        : 1;
  uint32_t Write       : 1;
  uint32_t Signal      : 1;
  uint32_t Persistant  : 1;
  uint32_t EdgeTrigger : 1;

  EventFlags_t(uint32_t flags = 0) { *reinterpret_cast<uint32_t*>(this) = flags; }

  void from_native(uint32_t flags);
  uint32_t to_native(void); // translates flags to the native platform flags
};
static_assert(sizeof(EventFlags_t) == sizeof(uint32_t), "EventFlags_t: bad size");

struct pollfd_t
{
  posix::fd_t fd;
  EventFlags_t events;
  EventFlags_t revents;

  pollfd_t(posix::fd_t _fd = 0, EventFlags_t _events = 0, EventFlags_t _revents = 0)
    : fd(_fd), events(_events), revents(_revents) { }
};


class EventBackend
{
public:
  struct features_t
  {
    uint8_t thread_specific     : 1; // Requires reinitialization when a new thread is created
    uint8_t edge_trigged        : 1; // Event method that allows edge-triggered events with EV_ET.
    uint8_t O1_efficient        : 1; // Event method where having one event triggered among many is [approximately] an O(1) operation.
                                     //   This excludes (for example) select and poll, which are approximately O(N) for N equal to the total number of possible events.
    uint8_t allows_non_sockets  : 1; // Event method that allows file descriptors as well as sockets.
  };

  /* Function to set up an event_base to use this backend.  It should
    create a new structure holding whatever information is needed to
    run the backend, and return it. */
  EventBackend(void);

  /** Function to clean up and free our data from the event_base. */
 ~EventBackend(void);

  /* Enable reading/writing on a given fd or signal.
      - 'events' = the events that we're trying to enable: one or more of EV_READ, EV_WRITE, EV_SIGNAL, and EV_ET.
      - 'old'    = those events that were enabled on this fd previously.
      - 'fdinfo' will be a structure associated with the fd by the evmap; its size is defined by the
        fdinfo field below.  It will be set to 0 the first time the fd is added.
   */
  bool watch(posix::fd_t fd, EventFlags_t events); // sets which events to montior in the watch queue

  bool remove(posix::fd_t fd); // remove from watch queue

  /* Function to implement the core of an event loop.  It must see which
     added events are ready, and cause event_active to be called for each
     active event (usually via event_io_active or such). */
  bool invoke(int timeout = -1);

  const std::vector<std::pair<posix::fd_t, EventFlags_t>>& data(void) const { return m_results; }

  // Bit-array of features that this backend has.
  const struct features_t features;

private:
  posix::fd_t m_pollfd;
  std::unordered_map<posix::fd_t, pollfd_t> m_queue; // watch queue
  std::vector<std::pair<posix::fd_t, EventFlags_t>> m_results;
  struct platform_dependant* m_data;
};

#endif // EVENTBACKEND_H
