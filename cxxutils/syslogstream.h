#ifndef SYSLOG_H
#define SYSLOG_H

// STL
#include <string>

// POSIX
#include <syslog.h>

// POSIX++
#include <climits>

namespace posix
{
  enum class priority : int
  {
    emergency = LOG_EMERG,    // system is unusable
    alert     = LOG_ALERT,    // action must be taken immediately
    critical  = LOG_CRIT,     // critical conditions
    error     = LOG_ERR,      // error conditions
    warning   = LOG_WARNING,  // warning conditions
    notice    = LOG_NOTICE,   // normal, but significant, condition
    info      = LOG_INFO,     // informational message
    debug     = LOG_DEBUG,    // debug-level message
  };

  enum class facility : int
  {
    kernel   = LOG_KERN,      // kernel messages
    user     = LOG_USER,      // user-level messages
    mail     = LOG_MAIL,      // mail system
    news     = LOG_NEWS,      // network news subsystem
    uucp     = LOG_UUCP,      // UUCP subsystem
    provider = LOG_DAEMON,    // system providers
    auth     = LOG_AUTH,      // security/authorization messages
    cron     = LOG_CRON,      // clock provider
    printer  = LOG_LPR,       // line printer subsystem
  };


  enum control
  {
    eom,
  };

  class SyslogStream
  {
  public:
    SyslogStream(void);
    static void open(const char* name, facility f = facility::provider) noexcept
      { ::openlog(name, LOG_PID | LOG_CONS | LOG_NOWAIT, int(f)); }
    static void close(void) noexcept { ::closelog(); }

    inline SyslogStream& operator << (priority p) noexcept { m_priority = p;  return *this; }
    inline SyslogStream& operator << (char c) noexcept { char tmp[2] = { c, 0 }; return operator << (tmp); }
    inline SyslogStream& operator << (char* str) noexcept { return operator << (const_cast<const char*>(str)); }
    inline SyslogStream& operator << (const std::string& str) noexcept { return operator << (str.c_str()); }

    template<typename T>
    inline SyslogStream& operator << (T val) noexcept { return operator << (std::to_string(val).c_str()); }

    SyslogStream& operator << (const char* arg) noexcept;
    SyslogStream& operator << (control cntl) noexcept;

  private:
    void clear(void);

    priority m_priority;
    char m_buffer[0x1000 + PATH_MAX];
    char m_tmpbuf[sizeof(m_buffer)];
    uint8_t m_argId;
  };

  static SyslogStream syslog;
}
#endif // SYSLOG_H
