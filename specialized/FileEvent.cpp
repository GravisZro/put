#include "FileEvent.h"

#if defined(__linux__)

// POSIX++
#include <cstring>
#include <cassert>

// Linux
#include <sys/inotify.h>

// PDTK
#include <cxxutils/vterm.h>

// file/directory flags
static constexpr uint32_t from_native_flags(const native_flags_t flags) noexcept
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

static constexpr native_flags_t to_native_flags(const uint32_t flags) noexcept
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
    flaw(fd == posix::invalid_descriptor, terminal::severe,,,
         "Unable to create an instance of inotify!: %s", std::strerror(errno))
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
    posix::fd_t wd;
    FileEvent::Flags_t flags;
  };

#define INOTIFY_EVENT_SIZE   (sizeof(inotify_event) + NAME_MAX + 1)
  return_data read(posix::fd_t wd) noexcept
  {
    static uint8_t inotifiy_buffer_data[INOTIFY_EVENT_SIZE * 16]; // queue has a minimum of size of 16 inotify events
    union {
      uint8_t* inpos;
      inotify_event* incur;
    };
    uint8_t* inend = inotifiy_buffer_data + posix::read(wd, inotifiy_buffer_data, sizeof(inotifiy_buffer_data)); // read data and locate it's end
    for(inpos = inotifiy_buffer_data; inpos < inend; inpos += sizeof(inotify_event) + incur->len) // iterate through the inotify events
      if(incur->wd == wd)
        return { incur->name, incur->wd, from_native_flags(incur->mask) };
    return { nullptr, 0, 0 };
  }

} FileEvent::s_platform;


FileEvent::FileEvent(const char* _file, Flags_t _flags) noexcept
  : m_flags(_flags), m_fd(posix::invalid_descriptor)
{
  std::memset(m_file, 0, sizeof(m_file));
  std::strcpy(m_file, _file);
  m_fd = s_platform.add(m_file, m_flags);
  EventBackend::add(m_fd, EventBackend::SimplePollReadFlags,
                    [this](posix::fd_t lambda_fd, native_flags_t) noexcept
                    {
                      platform_dependant::return_data data = s_platform.read(lambda_fd);
                      Object::enqueue(activated, data.name, data.flags);
                    });
}

FileEvent::~FileEvent(void) noexcept
{
  assert(EventBackend::remove(m_fd, EventBackend::SimplePollReadFlags));
  assert(s_platform.remove(m_fd));
}

#elif defined(__APPLE__)      /* Darwin 7+     */ || \
      defined(__FreeBSD__)    /* FreeBSD 4.1+  */ || \
      defined(__DragonFly__)  /* DragonFly BSD */ || \
      defined(__OpenBSD__)    /* OpenBSD 2.9+  */ || \
      defined(__NetBSD__)     /* NetBSD 2+     */


static constexpr native_flags_t composite_flag(uint16_t actions, int16_t filters, uint32_t flags) noexcept
  { return native_flags_t(actions) | (uint16_t(filters) << 16) | (flags << 32); }

// file flags
static constexpr uint32_t from_native_flags(const native_flags_t flags) noexcept
{
  return
      (flags & composite_flag(0, EVFILT_VNODE, NOTE_READ  ) ? FileEvent::ReadEvent    : 0) |
      (flags & composite_flag(0, EVFILT_VNODE, NOTE_WRITE ) ? FileEvent::WriteEvent   : 0) |
      (flags & composite_flag(0, EVFILT_VNODE, NOTE_ATTRIB) ? FileEvent::AttributeMod : 0) |
      (flags & composite_flag(0, EVFILT_VNODE, NOTE_RENAME) ? FileEvent::Moved        : 0) |
      (flags & composite_flag(0, EVFILT_VNODE, NOTE_DELETE) ? FileEvent::Deleted      : 0) ;
}

static constexpr native_flags_t to_native_flags(const uint32_t flags) noexcept
{
  return
      (flags & FileEvent::ReadEvent     ? composite_flag(0, EVFILT_VNODE, NOTE_READ  ) : 0) |
      (flags & FileEvent::WriteEvent    ? composite_flag(0, EVFILT_VNODE, NOTE_WRITE ) : 0) |
      (flags & FileEvent::AttributeMod  ? composite_flag(0, EVFILT_VNODE, NOTE_ATTRIB) : 0) |
      (flags & FileEvent::Moved         ? composite_flag(0, EVFILT_VNODE, NOTE_RENAME) : 0) |
      (flags & FileEvent::Deleted       ? composite_flag(0, EVFILT_VNODE, NOTE_DELETE) : 0) ;
}

FileEvent::FileEvent(const char* _file, Flags_t _flags) noexcept
  : m_flags(_flags), m_fd(posix::invalid_descriptor)
{
  std::memset(m_file, 0, sizeof(m_file));
  std::strcpy(m_file, _file);
  m_fd = posix::open(path, O_EVTONLY);
  EventBackend::add(m_fd, to_native_flags(m_flags), // connect FD with flags to signal
                    [this](posix::fd_t lambda_fd, native_flags_t lambda_flags) noexcept
                    { Object::enqueue_copy<const char*, Flags_t>(activated, m_file, from_native_flags(lambda_flags)); });
}

FileEvent::~FileEvent(void) noexcept
{
  EventBackend::remove(m_fd, to_native_flags(m_flags)); // disconnect FD with flags from signal
  posix::close(m_fd);
  m_fd = posix::invalid_descriptor;
}


# error No file event backend code exists in *BSD!  Please submit a patch!


#elif defined(__sun) && defined(__SVR4) // Solaris / OpenSolaris / OpenIndiana / illumos

# error No file event backend code exists in PDTK for Solaris / OpenSolaris / OpenIndiana / illumos!  Please submit a patch!
#elif defined(__minix) // MINIX
# error No process event backend code exists in PDTK for MINIX!  Please submit a patch!

#elif defined(__QNX__) // QNX
// QNX docs: http://www.qnx.com/developers/docs/7.0.0/index.html#com.qnx.doc.neutrino.devctl/topic/about.html
# error No file event backend code exists in PDTK for QNX!  Please submit a patch!

#elif defined(__hpux) // HP-UX

# error No file event backend code exists in PDTK for HP-UX!  Please submit a patch!

#elif defined(_AIX) // IBM AIX

# error No file event backend code exists in PDTK for IBM AIX!  Please submit a patch!

#elif defined(BSD)
# error Unrecognized BSD derivative!

#elif defined(__unix__)
# error Unrecognized UNIX variant!

#else
# error This platform is not supported.
#endif
