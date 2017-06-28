#ifndef SYSLOG_H
#define SYSLOG_H

// STL
#include <ostream>
#include <streambuf>

// POSIX
#include <sys/syslog.h>

namespace std
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
    daemon   = LOG_DAEMON,    // system daemons
    auth     = LOG_AUTH,      // security/authorization messages
    syslog   = LOG_SYSLOG,    // messages generated internally by syslogd
    printer  = LOG_LPR,       // line printer subsystem
    news     = LOG_NEWS,      // network news subsystem
    uucp     = LOG_UUCP,      // UUCP subsystem
    cron     = LOG_CRON,      // clock daemon
    authpriv = LOG_AUTHPRIV,  // security/authorization messages (private)
  };

  ostream& operator << (ostream& os, const priority& p);

  class syslogstreambuf : public basic_streambuf<char>
  {
  public:
    syslogstreambuf(const char *ident, facility f)
      : m_priority(priority::info)
    {
      m_buffer.reserve(4096);
      ::openlog(ident, 0, static_cast<int>(f));
    }

    ~syslogstreambuf(void)
    {
      sync();
      ::closelog();
    }

  protected:

    virtual int overflow( int c = EOF )
    {
      if (c == EOF)
        sync();
      else
        m_buffer += static_cast<char>(c & 0xFF);
      return c;
    }

    virtual int sync(void)
    {
      if (!m_buffer.empty())
      {
        ::syslog(static_cast<int>(m_priority), "%s", m_buffer.c_str());
        m_buffer.clear();
      }
      return 0;
    }

  private:
    friend ostream& operator << (ostream& os, const priority& p);

    string m_buffer;
    priority m_priority;
  };

  ostream& operator << (ostream& os, const priority& p)
  {
    static_cast<syslogstreambuf*>(os.rdbuf())->m_priority = p;
    return os;
  }


  static void sysloginit(ostream& os, const char *ident, facility f = facility::user)
  {
    os.rdbuf(new syslogstreambuf(ident, f));
  }

  static void sysloginit(const char *ident, facility f = facility::user)
  {
    sysloginit(clog, ident, f);
  }
}

#endif // SYSLOG_H
