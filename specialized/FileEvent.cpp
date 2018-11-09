#include "FileEvent.h"

// PUT
#include <specialized/osdetect.h>

#if defined(__linux__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,6,13) /* Linux 2.6.13+ */

// POSIX++
#include <cstring>
#include <cassert>

// Linux
#include <sys/inotify.h>

// PUT
#include <cxxutils/vterm.h>

// file/directory flags
static constexpr uint8_t from_native_flags(const native_flags_t flags) noexcept
{
  return
      (flags & IN_ACCESS      ? FileEvent::ReadEvent     : 0) |
      (flags & IN_MODIFY      ? FileEvent::WriteEvent    : 0) |
      (flags & IN_ATTRIB      ? FileEvent::AttributeMod  : 0) |
      (flags & IN_MOVE_SELF   ? FileEvent::Moved         : 0) |
      (flags & IN_DELETE_SELF ? FileEvent::Deleted       : 0) ;
  //data.flags.SubCreated   = flags & IN_CREATE      ? 1 : 0;
  //data.flags.SubMoved     = flags & IN_MOVE        ? 1 : 0;
  //data.flags.SubDeleted   = flags & IN_DELETE      ? 1 : 0;
}

static constexpr native_flags_t to_native_flags(const uint8_t flags) noexcept
{
  return
      (flags & FileEvent::ReadEvent    ? native_flags_t(IN_ACCESS     ) : 0) | // File was accessed (read) (*).
      (flags & FileEvent::WriteEvent   ? native_flags_t(IN_MODIFY     ) : 0) | // File was modified (*).
      (flags & FileEvent::AttributeMod ? native_flags_t(IN_ATTRIB     ) : 0) | // Metadata changed, e.g., permissions, timestamps, extended attributes, link count (since Linux 2.6.25), UID, GID, etc. (*).
      (flags & FileEvent::Moved        ? native_flags_t(IN_MOVE_SELF  ) : 0) | // Watched File was moved.
      (flags & FileEvent::Deleted      ? native_flags_t(IN_DELETE_SELF) : 0);  // Watched File was deleted.
//        (flags & FileEvent::SubCreated   ? native_flags_t(IN_CREATE     ) : 0) | // File created in watched dir.
//        (flags & FileEvent::SubMoved     ? native_flags_t(IN_MOVE       ) : 0) | // File moved in watched dir.
//        (flags & FileEvent::SubDeleted   ? native_flags_t(IN_DELETE     ) : 0) ; // File deleted in watched dir.
}

struct FileEvent::platform_dependant // file notification (inotify)
{
  posix::fd_t fd;

  platform_dependant(void) noexcept
    : fd(posix::invalid_descriptor)
  {
    fd = ::inotify_init();
    flaw(fd == posix::invalid_descriptor,
         terminal::severe,,,
         "Unable to create an instance of inotify!: %s", std::strerror(errno));
    posix::fcntl(fd, F_SETFD, FD_CLOEXEC); // close on exec*()
  }

  ~platform_dependant(void) noexcept
  {
    posix::close(fd);
    fd = posix::invalid_descriptor;
  }

  posix::fd_t add(const char* path, FileEvent::Flags_t flags) noexcept
  {
    return ::inotify_add_watch(fd, path, to_native_flags(flags));
  }

  bool remove(posix::fd_t wd) noexcept
  {
    return ::inotify_rm_watch(fd, wd) == posix::success_response;
  }

  struct return_data
  {
    const char* name;
    FileEvent::Flags_t flags;
  };

#define INOTIFY_EVENT_SIZE   (sizeof(inotify_event) + NAME_MAX + 1)
  return_data read(posix::fd_t wd) const noexcept
  {
    static uint8_t buffer[INOTIFY_EVENT_SIZE * 16]; // buffer (holds a minimum of 16 inotify events)
    union {
      uint8_t* pos;
      inotify_event* event;
    };
    uint8_t* end;
    pos = buffer;
    do
    {
      end = pos + posix::read(wd, pos, sizeof(buffer) - (buffer - pos)); // read new chunk of inotify events
      while(pos < end) // iterate through the inotify events
      {
        if(event->wd == wd)
          return { event->name, from_native_flags(event->mask) };
        if(pos + sizeof(inotify_event) + event->len >= end) // check if next entry exceeds buffer
        {
          std::memmove(buffer, pos, end - pos); // move partial entry
          pos = buffer + (end - pos); // move to end of partial entry
          end = pos; // ensure exit
        }
        else
          pos += sizeof(inotify_event) + event->len; // move to next event
      }
    } while(end != buffer); // while there is still more to read
    return { nullptr, 0 }; // read the entire buffer and the event was never found!
  }

} FileEvent::s_platform;


FileEvent::FileEvent(const std::string& _file, Flags_t _flags) noexcept
  : m_file(_file),
    m_flags(_flags),
    m_fd(posix::invalid_descriptor)
{
  m_fd = s_platform.add(m_file.c_str(), m_flags);
  EventBackend::add(m_fd, EventBackend::SimplePollReadFlags,
                    [this](posix::fd_t lambda_fd, native_flags_t) noexcept
                    {
                      platform_dependant::return_data data = s_platform.read(lambda_fd);
                      assert(data.name != nullptr); // entry was found
                      assert(m_flags & data.flags); // flags match
                      assert(m_file == data.name); // name matches
                      if(data.name != nullptr) // just in case (should never be false)
                        Object::enqueue_copy<std::string, Flags_t>(activated, data.name, data.flags);
                    });
}

FileEvent::~FileEvent(void) noexcept
{
  assert(EventBackend::remove(m_fd, EventBackend::SimplePollReadFlags));
  assert(s_platform.remove(m_fd));
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

// file flags
static constexpr uint8_t from_native_flags(const native_flags_t flags) noexcept
{
  return
#if defined(NOTE_READ)
      (flag_subset(flags, composite_flag(0, EVFILT_VNODE, NOTE_READ  )) ? FileEvent::ReadEvent    : 0) |
#endif
      (flag_subset(flags, composite_flag(0, EVFILT_VNODE, NOTE_WRITE )) ? FileEvent::WriteEvent   : 0) |
      (flag_subset(flags, composite_flag(0, EVFILT_VNODE, NOTE_ATTRIB)) ? FileEvent::AttributeMod : 0) |
      (flag_subset(flags, composite_flag(0, EVFILT_VNODE, NOTE_RENAME)) ? FileEvent::Moved        : 0) |
      (flag_subset(flags, composite_flag(0, EVFILT_VNODE, NOTE_DELETE)) ? FileEvent::Deleted      : 0) ;
}

static constexpr native_flags_t to_native_flags(const uint8_t flags) noexcept
{
  return
#if defined(NOTE_READ)
      (flags & FileEvent::ReadEvent     ? composite_flag(0, EVFILT_VNODE, NOTE_READ  ) : 0) |
#endif
      (flags & FileEvent::WriteEvent    ? composite_flag(0, EVFILT_VNODE, NOTE_WRITE ) : 0) |
      (flags & FileEvent::AttributeMod  ? composite_flag(0, EVFILT_VNODE, NOTE_ATTRIB) : 0) |
      (flags & FileEvent::Moved         ? composite_flag(0, EVFILT_VNODE, NOTE_RENAME) : 0) |
      (flags & FileEvent::Deleted       ? composite_flag(0, EVFILT_VNODE, NOTE_DELETE) : 0) ;
}

FileEvent::FileEvent(const std::string& _file, Flags_t _flags) noexcept
  : m_file(_file),
    m_flags(_flags),
    m_fd(posix::invalid_descriptor)
{
  m_fd = posix::open(m_file.c_str(), O_EVTONLY);
  EventBackend::add(m_fd, to_native_flags(m_flags), // connect FD with flags to signal
                    [this](posix::fd_t lambda_fd, native_flags_t lambda_flags) noexcept
                    { Object::enqueue_copy<std::string, Flags_t>(activated, m_file, from_native_flags(lambda_flags)); });
}

FileEvent::~FileEvent(void) noexcept
{
  EventBackend::remove(m_fd, to_native_flags(m_flags)); // disconnect FD with flags from signal
  posix::close(m_fd);
  m_fd = posix::invalid_descriptor;
}

#elif defined(__solaris__) /* Solaris */
# error No file event backend code exists in PUT for Solaris!  Please submit a patch!

#elif defined(__QNX__) // QNX
// QNX docs: http://www.qnx.com/developers/docs/7.0.0/index.html#com.qnx.doc.neutrino.devctl/topic/about.html
# error No file event backend code exists in PUT for QNX!  Please submit a patch!

#elif defined(__unix__)
# error No file event backend code exists in PUT for this UNIX!  Please submit a patch!

#else
# error This platform is not supported.
#endif
