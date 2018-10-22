#include "syslogstream.h"

#include <cstring>
#include <cassert>

namespace posix
{
  SyslogStream::SyslogStream(void) { clear(); }

  SyslogStream& SyslogStream::operator << (const char* arg) noexcept
  {
    constexpr char lookup_table[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                                      'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                                      'U', 'V', 'W', 'X', 'Y', 'Z' };
    if(!m_argId)
      std::strncpy(m_buffer, arg, sizeof(m_buffer));
    else if(m_argId > 0 && size_t(m_argId) < sizeof(lookup_table))
    {

      ssize_t buffer_remaining = sizeof(m_tmpbuf);
      const size_t arglen = std::strlen(arg);
      std::memset(m_tmpbuf, 0, sizeof(m_tmpbuf));

      char seach_token[3] = { '%', '0', '\0' };
      seach_token[1] = lookup_table[m_argId];

      char* lastpos = m_buffer;
      for(char* pos = nullptr;
          (pos = std::strstr(lastpos, seach_token)) != NULL;
          lastpos = pos + 2)
      {
        ssize_t slice = pos - lastpos;
        buffer_remaining -= slice;
        if(buffer_remaining > 0)
          std::strncat(m_tmpbuf, lastpos, std::size_t(slice));

        buffer_remaining -= arglen;
        if(buffer_remaining > 0)
          std::strncat(m_tmpbuf, arg, arglen);
      }
      if(buffer_remaining > 0)
        std::strncat(m_tmpbuf, lastpos, size_t(buffer_remaining));
      std::strncpy(m_buffer, m_tmpbuf, sizeof(m_buffer));
    }

    ++m_argId;
    return *this;
  }

  SyslogStream& SyslogStream::operator << (control cntl) noexcept
  {
    assert(cntl == control::eom);
    ::syslog(int(m_priority), "%s", m_buffer);
    clear();
    return *this;
  }

  inline void SyslogStream::clear(void)
  {
    std::memset(m_buffer, 0, sizeof(m_buffer));
    m_priority = priority::info;
    m_argId = 0;
  }
}
