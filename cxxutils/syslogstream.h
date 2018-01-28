#ifndef SYSLOG_H
#define SYSLOG_H

// STL
#include <ostream>
#include <streambuf>
#include <string>

// POSIX
#include <syslog.h>

namespace posix
{
  using ::openlog;
  using ::closelog;
  //using ::syslog;

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
    daemon   = LOG_DAEMON,    // system daemons
    auth     = LOG_AUTH,      // security/authorization messages
    printer  = LOG_LPR,       // line printer subsystem
    news     = LOG_NEWS,      // network news subsystem
    uucp     = LOG_UUCP,      // UUCP subsystem
    cron     = LOG_CRON,      // clock daemon
    authpriv = LOG_AUTHPRIV,  // security/authorization messages (private)
  };

  enum control
  {
    eol,
    eom,
  };

  class SyslogStream
  {
  public:
    static void open(const char* name, facility f = facility::daemon)
      { posix::openlog(name, LOG_PID | LOG_CONS | LOG_NOWAIT, int(f)); }
    static void close(void) { posix::closelog(); }

    inline SyslogStream& operator << (priority p) { m_priority = p;  return *this; }
    inline SyslogStream& operator << (char c) { m_buffer.push_back(c); return *this; }
    inline SyslogStream& operator << (char* d) { return operator << (const_cast<const char*>(d)); }
    inline SyslogStream& operator << (const char* d) { if(d == nullptr) { m_buffer.append("nullptr"); } else { m_buffer.append(d); } return *this; }
    inline SyslogStream& operator << (const std::string& d) { m_buffer.append(d);  return *this; }

    template<typename T>
    inline SyslogStream& operator << (T val) { m_buffer.append(std::to_string(val)); return *this; }

    SyslogStream& operator << (control cntl)
    {
      switch(cntl)
      {
        case eol:
          m_buffer.push_back('\n');
          break;
        case eom:
          ::syslog(int(m_priority), "%s", m_buffer.c_str());
          m_buffer.clear();
          m_priority = priority::info;
          break;
      }
      return *this;
    }

  private:
    priority m_priority;
    std::string m_buffer;
  };

  static SyslogStream syslog;
}
#endif // SYSLOG_H
