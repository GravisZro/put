#ifndef SYSLOG_H
#define SYSLOG_H

// STL
#include <string>
#include <list>

// POSIX
#include <syslog.h>

// PUT
#include <cxxutils/posix_helpers.h>

namespace posix
{
  enum control { eom = EOF, };
}

#ifndef MESSAGE_BUFFER_SIZE
#define MESSAGE_BUFFER_SIZE (0x1000 + PATH_MAX)
#endif

class ErrorMessageStream
{
public:

  ErrorMessageStream(void) noexcept;
  virtual ~ErrorMessageStream(void) noexcept = default;

  inline ErrorMessageStream& operator << (char c) noexcept { char tmp[2] = { c, 0 }; return operator << (tmp); }
  inline ErrorMessageStream& operator << (char* str) noexcept { return operator << (const_cast<const char*>(str)); }
  inline ErrorMessageStream& operator << (const std::string& str) noexcept { return operator << (str.c_str()); }

  template<typename T>
  inline ErrorMessageStream& operator << (T val) noexcept { return operator << (std::to_string(val).c_str()); }

  ErrorMessageStream& operator << (const char* arg) noexcept;
  ErrorMessageStream& operator << (posix::control cntl) noexcept;

protected:
  virtual void purge_buffer(void) noexcept;
  virtual void publish_buffer(void) noexcept = 0;

  char m_buffer[MESSAGE_BUFFER_SIZE];
  char m_tmpbuf[sizeof(m_buffer)];
  uint8_t m_argId;
};

class ErrorLogStream : public ErrorMessageStream
{
public:
  ErrorLogStream(void) noexcept;

  bool empty(void) const noexcept { return m_destination.empty(); }
  void clear(void) noexcept { m_destination.clear(); }
  std::list<std::string>& messages(void) noexcept { return m_destination; }

private:
  void publish_buffer(void) noexcept;
  std::list<std::string> m_destination;
};

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

  class SyslogStream : public ErrorMessageStream
  {
  public:
    SyslogStream(void) noexcept;

    static void open(const char* name, facility f = facility::provider) noexcept
      { ::openlog(name, LOG_PID | LOG_CONS | LOG_NOWAIT, int(f)); }
    static void close(void) noexcept { ::closelog(); }

    inline ErrorMessageStream& operator << (priority p) noexcept { m_priority = p;  return *this; }
  private:
    void purge_buffer(void) noexcept;
    void publish_buffer(void) noexcept;
    priority m_priority;
  };

  static SyslogStream syslog;
}
#endif // SYSLOG_H
