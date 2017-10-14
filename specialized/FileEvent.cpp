#include "FileEvent.h"

#if defined(__linux__)

// POSIX++
#include <cstring>
#include <cassert>

// Linux
#include <sys/inotify.h>

// PDTK
#include <cxxutils/vterm.h>

struct FileEvent::platform_dependant // file notification (inotify)
{
  posix::fd_t fd;

  // file/directory flags
  static constexpr uint8_t from_native_flags(const uint32_t flags) noexcept
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

  static constexpr uint32_t to_native_flags(const uint8_t flags) noexcept
  {
    return
        (flags & FileEvent::ReadEvent    ? uint32_t(IN_ACCESS     ) : 0) | // File was accessed (read) (*).
        (flags & FileEvent::WriteEvent   ? uint32_t(IN_MODIFY     ) : 0) | // File was modified (*).
        (flags & FileEvent::AttributeMod ? uint32_t(IN_ATTRIB     ) : 0) | // Metadata changed, e.g., permissions, timestamps, extended attributes, link count (since Linux 2.6.25), UID, GID, etc. (*).
        (flags & FileEvent::Moved        ? uint32_t(IN_MOVE_SELF  ) : 0) | // Watched File was moved.
        (flags & FileEvent::Deleted      ? uint32_t(IN_DELETE_SELF) : 0);  // Watched File was deleted.
//        (flags & FileEvent::SubCreated   ? uint32_t(IN_CREATE     ) : 0) | // File created in watched dir.
//        (flags & FileEvent::SubMoved     ? uint32_t(IN_MOVE       ) : 0) | // File moved in watched dir.
//        (flags & FileEvent::SubDeleted   ? uint32_t(IN_DELETE     ) : 0) ; // File deleted in watched dir.
  }

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
  static_assert(sizeof(m_file) == PATH_MAX, "whoops!");
  std::memset(m_file, 0, sizeof(m_file));
  std::strcpy(m_file, _file);
  m_fd = s_platform.add(m_file, m_flags);
  EventBackend::add(m_fd, Event::Readable,
                    [this](posix::fd_t lambda_fd, Event::Flags_t lambda_flags) noexcept
                    {
                      assert(lambda_flags == Event::Readable);
                      platform_dependant::return_data data = s_platform.read(lambda_fd);
                      assert(m_fd == data.wd);
                      assert(m_flags == data.flags);
                      Object::enqueue(activated, data.wd, data.name, data.flags);
                    });
}

FileEvent::~FileEvent(void) noexcept
{
  assert(EventBackend::remove(m_fd, Event::Readable));
  assert(s_platform.remove(m_fd));
}

#elif defined(__APPLE__)      /* Darwin 7+     */ || \
      defined(__FreeBSD__)    /* FreeBSD 4.1+  */ || \
      defined(__DragonFly__)  /* DragonFly BSD */ || \
      defined(__OpenBSD__)    /* OpenBSD 2.9+  */ || \
      defined(__NetBSD__)     /* NetBSD 2+     */

# error No file event backend code exists in *BSD!  Please submit a patch!


constexpr uint32_t to_native_flags(const EventFlags_t& flags)
{
  return
      (flags.ExecEvent    ? uint32_t(NOTE_EXEC    ) : 0) | // Process called exec*()
      (flags.ExitEvent    ? uint32_t(NOTE_EXIT    ) : 0) | // Process exited
      (flags.ForkEvent    ? uint32_t(NOTE_FORK    ) : 0) | // Process forked
}


// FD flags
inline EventData_t from_kevent(const struct kevent& ev) noexcept
{
    case EVFILT_PROC:
    // process flags
      data.flags.ExecEvent      = ev.flags & NOTE_EXEC      ? 1 : 0;
      data.flags.ExitEvent      = ev.flags & NOTE_EXIT      ? 1 : 0;
      data.flags.ForkEvent      = ev.flags & NOTE_FORK      ? 1 : 0;
      break;
  }


  return data;
}


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
