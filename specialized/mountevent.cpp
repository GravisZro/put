#include "mountevent.h"

// POSIX++
#include <climits>

// PUT
#include <specialized/osdetect.h>
#include <specialized/mountpoints.h>

#if defined(__linux__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,6,30) /* Linux 2.6.30+ */
# if defined(FORCE_POSIX_POLL)
// POSIX
#  include <poll.h>
#  define PROC_MOUNTS_FLAGS   (POLLERR | POLLPRI)
# else
// Linux
#  include <sys/epoll.h>
#  define PROC_MOUNTS_FLAGS   (EPOLLERR | EPOLLPRI)
# endif
#elif defined(__unix__)   /* Generic UNIX */
// PUT
# include <specialized/timerevent.h>
#else
# error Unsupported platform! >:(
#endif

#include <algorithm> // for find

MountEvent::MountEvent(void) noexcept
  : m_timer(nullptr)
{
    mount_table(m_table); // initial scan

    EventBackend::callback_t comparison_func =
        [this](posix::fd_t, native_flags_t) noexcept
        {
          std::list<struct fsentry_t> new_table;
          if(mount_table(new_table))
          {
            for(auto& entry : new_table)
              if(std::find(m_table.begin(), m_table.end(), entry) == m_table.end())
                Object::enqueue_copy<std::string, std::string>(mounted, entry.device, entry.path);

            for(auto& entry : m_table)
              if(std::find(new_table.begin(), new_table.end(), entry) == new_table.end())
                Object::enqueue_copy<std::string, std::string>(unmounted, entry.device, entry.path);

            m_table = new_table;
          }
        };

#if defined(PROC_MOUNTS_FLAGS) /* Linux 2.6.30+ */
    if(procfs_path != nullptr) // safety check
    {
      char proc_mounts[PATH_MAX] = { 0 };
      posix::sscanf(proc_mounts, "%s/mounts", procfs_path);
      m_fd = posix::open(proc_mounts, O_RDONLY | O_NONBLOCK);
      EventBackend::add(m_fd, PROC_MOUNTS_FLAGS, comparison_func); // connect FD with flags to comparison_func
    }
#elif defined(__unix__)   /* Generic UNIX */
    m_timer = new TimerEvent();
    m_timer->start(seconds(10), true); // once per 10 seconds
    Object::connect(m_timer->expired, fslot_t<void>([comparison_func](void) noexcept { comparison_func(0,0); }));
#endif
}

MountEvent::~MountEvent(void) noexcept
{
#if defined(PROC_MOUNTS_FLAGS) /* Linux 2.6.30+ */
  EventBackend::remove(m_fd, PROC_MOUNTS_FLAGS); // disconnect FD with flags from signal
  posix::close(m_fd);
#elif defined(__unix__)   /* Generic UNIX */
  if(m_timer != nullptr)
    delete m_timer;
  m_timer = nullptr;
#endif
}

