#ifndef SYSLOG_H
#define SYSLOG_H

// STL
#include <ostream>
#include <streambuf>
#include <string>

// POSIX
#include <syslog.h>

// POSIX++
#include <climits>
#include <cstring>
#include <cassert>

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
    user     = LOG_USER,      // random user-level messages
    mail     = LOG_MAIL,      // mail system
    provider = LOG_DAEMON,    // system providers
    auth     = LOG_AUTH,      // security/authorization messages
    printer  = LOG_LPR,       // line printer subsystem
    news     = LOG_NEWS,      // network news subsystem
    uucp     = LOG_UUCP,      // UUCP subsystem
    cron     = LOG_CRON,      // clock provider
    authpriv = LOG_AUTHPRIV,  // security/authorization messages (private)
  };

  enum control
  {
    eom,
  };

  class SyslogStream
  {
  public:
    SyslogStream(void) { clear(); }
    static void open(const char* name, facility f = facility::provider) noexcept
      { ::openlog(name, LOG_PID | LOG_CONS | LOG_NOWAIT, int(f)); }
    static void close(void) noexcept { ::closelog(); }

    inline SyslogStream& operator << (priority p) noexcept { m_priority = p;  return *this; }
    inline SyslogStream& operator << (char c) noexcept { char tmp[2] = { c, 0 }; return operator << (tmp); }
    inline SyslogStream& operator << (char* str) noexcept { return operator << (const_cast<const char*>(str)); }
    inline SyslogStream& operator << (const std::string& str) noexcept { return operator << (str.c_str()); }

    template<typename T>
    inline SyslogStream& operator << (T val) noexcept { return operator << (std::to_string(val).c_str()); }

    inline SyslogStream& operator << (const char* arg) noexcept
    {
      if(m_argId == '0')
        std::strncpy(m_buffer, arg, sizeof(m_buffer));
      else
      {
        size_t tmplen = 0;
        const size_t arglen = std::strlen(arg);
        std::memset(m_tmpbuf, 0, sizeof(m_tmpbuf));

        char seach_token[3] = { '%', '0', '\0' };
        seach_token[1] = m_argId;

        char* pos = m_buffer;
        char* lastpos = m_buffer;
        while((pos = std::strtok(pos + 2, seach_token)) != nullptr)
        {
          std::strncat(m_tmpbuf, lastpos, size_t(pos - lastpos));
          std::strncat(m_tmpbuf, arg, sizeof(m_tmpbuf) - tmplen - arglen);
          tmplen += arglen;
          lastpos = pos;
        }
        std::strncpy(m_buffer, m_tmpbuf, sizeof(m_buffer));
      }

      ++m_argId;
      return *this;
    }

    SyslogStream& operator << (control cntl) noexcept
    {
      assert(cntl == control::eom);
      ::syslog(int(m_priority), "%s", m_buffer);
      clear();
      return *this;
    }

  private:
    void clear(void)
    {
      std::memset(m_buffer, 0, sizeof(m_buffer));
      m_priority = priority::info;
      m_argId = '0';
    }

    priority m_priority;
    char m_buffer[0x1000 + PATH_MAX];
    char m_tmpbuf[sizeof(m_buffer)];
    char m_argId;
  };

  static SyslogStream syslog;
}
#endif // SYSLOG_H
