#include "stringtoken.h"

#include <cassert>

StringToken::StringToken(const char* origin) noexcept
  : m_pos(origin)
{
  assert(m_pos != nullptr);
}

const char* StringToken::next(char token) noexcept
{
  if(m_pos != nullptr && *m_pos)
    while(++m_pos)
      if(*m_pos == token)
        return m_pos;
  return nullptr;
}

const char* StringToken::next(const char* tokens) noexcept
{
  if(m_pos != nullptr && *m_pos)
    while(++m_pos)
      for(const char* token = tokens; *token; ++token)
        if(*m_pos == *token)
          return m_pos;
  return nullptr;
}
