#include "vfifo.h"

vfifo::vfifo(const vfifo& other) noexcept
  :
    m_virt_begin(0),
    m_virt_end(other.size()),
    m_ok(true)
{
  m_data = posix::malloc(other.size());
}

bool vfifo::allocate(posix::ssize_t length) noexcept
{
  if(discarded() && size())
    posix::memmove(m_data, data(), size());

  if(length > capacity())
  {
    m_data = posix::realloc(m_data, length);
    m_capacity = length;
  }
  return m_data != NULL;
}

void vfifo::reset(void) noexcept
{
  m_virt_end = m_virt_begin = 0;
  clearError();
}

// make sure strings are read in properly!
template<> vfifo& vfifo::operator << (const std::string& arg) noexcept
{
  if(posix::ssize_t(arg.size()) > unused())
    allocate(capacity() * 2);
  serialize_arr(arg.data(), uint16_t(arg.size()));
  return *this;
}

// make sure strings are read in properly!
template<> vfifo& vfifo::operator << (const std::wstring& arg) noexcept
{
  if(posix::ssize_t(arg.size()) > unused())
    allocate(capacity() * 2);
  serialize_arr(arg.data(), uint16_t(arg.size()));
  return *this;
}
