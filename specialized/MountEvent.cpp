#include "MountEvent.h"

#if defined(__linux__)
#include <linux/version.h>
#elif !defined(KERNEL_VERSION)
#define LINUX_VERSION_CODE 0
#define KERNEL_VERSION(a, b, c) 0
#endif

#if defined(__linux__) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30) /* Linux 2.6.30+ */
// Linux
# include <sys/epoll.h>
#elif defined(__unix__) || defined(__unix)      /* Generic UNIX */ || \
      (defined(__APPLE__) && defined(__MACH__)) /* Darwin       */
// PDTK
# include "TimerEvent.h"
#else
# error Unsupported platform! >:(
#endif

// POSIX++
#include <climits>
#include <cstring>

#ifndef MOUNT_TABLE_FILE
#define MOUNT_TABLE_FILE  "/etc/mtab"
#endif

MountEvent::MountEvent(void) noexcept
  : m_timer(nullptr)
{
    parse_table(m_table, MOUNT_TABLE_FILE);

    EventBackend::callback_t comparison_func =
        [this](posix::fd_t, native_flags_t) noexcept
        {
          std::set<struct fsentry_t> new_table;
          parse_table(new_table, MOUNT_TABLE_FILE);

          for(auto& entry : new_table)
            if(m_table.find(entry) == m_table.end())
              Object::enqueue_copy<std::string, std::string>(mounted, std::string(entry.device), std::string(entry.path));

          for(auto& entry : m_table)
            if(new_table.find(entry) == new_table.end())
              Object::enqueue_copy<std::string, std::string>(unmounted, std::string(entry.device), std::string(entry.path));

          m_table = new_table;
        };

#if defined(__linux__) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30) /* Linux 2.6.30+ */
    m_fd = posix::open(MOUNT_TABLE_FILE, O_RDONLY | O_NONBLOCK);
    EventBackend::add(m_fd, EPOLLERR | EPOLLPRI, comparison_func); // connect FD with flags to comparison_func
#elif defined(__unix__) || defined(__unix)      /* Generic UNIX */ || \
      (defined(__APPLE__) && defined(__MACH__)) /* Darwin       */
    m_timer = new TimerEvent();
    m_timer->start(10000000, 10000000); // once per 10 seconds
    Object::connect(m_timer->expired, fslot_t<void>([this](void) noexcept { comparison_func(0,0) }));
#endif
}

MountEvent::~MountEvent(void) noexcept
{
#if defined(__linux__) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30) /* Linux 2.6.30+ */
  EventBackend::remove(m_fd, EPOLLERR | EPOLLPRI); // disconnect FD with flags from signal
  posix::close(m_fd);
#elif defined(__unix__) || defined(__unix)      /* Generic UNIX */ || \
      (defined(__APPLE__) && defined(__MACH__)) /* Darwin       */
  if(m_timer != nullptr)
    delete m_timer;
  m_timer = nullptr;
#endif
}

