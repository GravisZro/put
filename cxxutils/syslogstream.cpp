#include "syslogstream.h"

#include <assert.h>

ErrorMessageStream::ErrorMessageStream(void) noexcept
{
  ErrorMessageStream::purge_buffer();
}

ErrorMessageStream& ErrorMessageStream::operator << (const char* arg) noexcept
{
  constexpr char lookup_table[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                                    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                                    'U', 'V', 'W', 'X', 'Y', 'Z' };
  if(!m_argId)
    posix::strncpy(m_buffer, arg, sizeof(m_buffer));
  else if(m_argId > 0 && size_t(m_argId) < sizeof(lookup_table))
  {

    ssize_t buffer_remaining = sizeof(m_tmpbuf);
    const size_t arglen = posix::strlen(arg);
    posix::memset(m_tmpbuf, 0, sizeof(m_tmpbuf));

    char seach_token[3] = { '%', '0', '\0' };
    seach_token[1] = lookup_table[m_argId];

    char* lastpos = m_buffer;
    for(char* pos = nullptr;
        (pos = posix::strstr(lastpos, seach_token)) != NULL;
        lastpos = pos + 2)
    {
      ssize_t slice = pos - lastpos;
      buffer_remaining -= slice;
      if(buffer_remaining > 0)
        posix::strncat(m_tmpbuf, lastpos, posix::size_t(slice));

      buffer_remaining -= arglen;
      if(buffer_remaining > 0)
        posix::strncat(m_tmpbuf, arg, arglen);
    }
    if(buffer_remaining > 0)
      posix::strncat(m_tmpbuf, lastpos, size_t(buffer_remaining));
    posix::strncpy(m_buffer, m_tmpbuf, sizeof(m_buffer));
  }

  ++m_argId;
  return *this;
}

ErrorMessageStream& ErrorMessageStream::operator << (posix::control cntl) noexcept
{
  assert(cntl == posix::eom);
  m_buffer[sizeof(m_buffer) - 1] = '\0'; // forcibly null terminate string for safety
  publish_buffer();
  purge_buffer();
  return *this;
}

ErrorLogStream::ErrorLogStream(void) noexcept { }

inline void ErrorMessageStream::purge_buffer(void) noexcept
{
  posix::memset(m_buffer, 0, sizeof(m_buffer));
  m_argId = 0;
}

inline void ErrorLogStream::publish_buffer(void) noexcept
{
  m_destination.emplace_back(m_buffer);
}

posix::SyslogStream::SyslogStream(void) noexcept { }

inline void posix::SyslogStream::purge_buffer(void) noexcept
{
  m_priority = priority::info;
  ErrorMessageStream::purge_buffer();
}

inline void posix::SyslogStream::publish_buffer(void) noexcept
{
  ::syslog(int(m_priority), "%s", m_buffer);
}
