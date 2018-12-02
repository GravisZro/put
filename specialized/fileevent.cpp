#include "fileevent.h"

// PUT
#include <specialized/osdetect.h>

#if defined(__linux__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,6,13) /* Linux 2.6.13+ */

// POSIX++
# include <cstring>
# include <cassert>

// Linux
# include <sys/inotify.h>

// PUT
# include <cxxutils/vterm.h>

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

# include <sys/event.h> // kqueue

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

#else
// POSIX++
# include <cstring>
# include <cassert>

// POSIX
# include <sys/stat.h>

// PUT
# include <cxxutils/vterm.h>
# include <specialized/TimerEvent.h>

enum {
  Read = 0,
  Write = 1,
};

template<typename T>
static inline bool data_identical(T& a, T& b) noexcept
{ return std::memcmp(&a, &b, sizeof(T)) == 0; }

struct FileEvent::platform_dependant // file notification (TimerEvent)
{
  struct eventinfo_t
  {
    posix::fd_t fd[2]; // two fds for pipe based communication
    const char* file;
    Flags_t flags;
    struct stat status;
    TimerEvent timer;
    uint32_t test_count;
  };

  std::unordered_map<posix::fd_t, eventinfo_t> events;

  posix::fd_t add(const char* file, Flags_t flags) noexcept
  {
    eventinfo_t data;
    data.file = file;
    data.flags = flags;
    data.test_count = 0;

    if(::stat(file, &data.status) == posix::error_response ||
       !posix::pipe(data.fd))
      return posix::invalid_descriptor;

    posix::fd_t& readfd = data.fd[Read];
    posix::fcntl(readfd, F_SETFD, FD_CLOEXEC); // close on exec*()
    posix::donotblock(readfd); // don't block

    auto pair = events.emplace(readfd, data);
    if(!pair.second) // emplace failed
      return posix::invalid_descriptor;

    Object::connect(data.timer.expired,
                    Object::fslot_t<void>([this, pair](void) noexcept
                      { update(pair.first->second); }));

    data.timer.start(1000, true);
    return readfd;
  }

  void update(eventinfo_t& data) noexcept
  {
    Flags_t flags;
    struct stat status;
    if(::stat(data.file, &status) == posix::success_response &&
       !data_identical(data.status, status))
    {
      flags.ReadEvent    = data_identical(data.status.st_atim, status.st_atim) ? 0 : 1;
      flags.WriteEvent   = data_identical(data.status.st_mtim, status.st_mtim) ? 0 : 1;
      flags.AttributeMod = data.status.st_mode == status.st_mode &&
                           data.status.st_uid  == status.st_uid &&
                           data.status.st_gid  == status.st_gid &&
                           data.status.st_rdev == status.st_rdev ? 0 : 1;
      flags.Moved        = data.status.st_dev  == status.st_dev &&
                           data.status.st_ino  == status.st_ino  ? 0 : 1;
    }
    else
    {
      switch(errno)
      {
        case EACCES: flags.AttributeMod = 1; break;
        case ENOTDIR:
        case ENOENT: flags.Deleted = 1; break; // file may be moved but assume it's deleted
      }
    }
    if(flags)
    {
      data.test_count = 0;
      posix::write(data.fd[Write], &flags, sizeof(flags));
    }
    if(flags.Deleted)
      remove(data.fd[Read]);
    else
    {
      if(data.test_count == 0)
        data.timer.start(seconds(1), true); // test once per second
      else if(data.test_count == 10)
        data.timer.start(seconds(10), true); // test every 10 seconds
      else if(data.test_count == 100)
        data.timer.start(seconds(100), true); // test every 100 seconds
      data.test_count++;
    }
  }

  bool remove(posix::fd_t fd) noexcept
  {
    auto iter = events.find(fd);
    if(iter != events.end())
    {
      posix::close(iter->second.fd[Read]);
      posix::close(iter->second.fd[Write]);
      events.erase(iter);
      return true;
    }
    return false;
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
                      Flags_t event_flags, new_flags;
                      while(posix::read(lambda_fd, &new_flags, sizeof(event_flags)) != posix::error_response) // read all events
                        event_flags = event_flags | new_flags; // compose new flag result
                      assert(m_flags & event_flags); // at least one flag matches
                      Object::enqueue(activated, m_file, event_flags);
                    });
}

FileEvent::~FileEvent(void) noexcept
{
  assert(EventBackend::remove(m_fd, EventBackend::SimplePollReadFlags));
  s_platform.remove(m_fd); // may already be deleted
}
#endif
