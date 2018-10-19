#include "vfifo.h"

vfifo& vfifo::operator=(const vfifo& other) noexcept
{
  if (this != &other)
  {
    m_data       = const_cast<vfifo&>(other).m_data;
    m_virt_begin = other.m_virt_begin;
    m_virt_end   = other.m_virt_end;
    m_capacity   = other.m_capacity;
    m_ok         = other.m_ok;
    m_external   = other.m_external;
  }
  return *this;
}

bool vfifo::allocate(posix::ssize_t length) noexcept
{
  if(m_external)
    return m_ok = false;

  if(length <= capacity()) // reducing the allocated size is not allowed!
    return false;
  char* new_data = static_cast<char*>(::malloc(posix::size_t(length)));
  if(new_data == nullptr)
    return false;

  std::memset(new_data, 0, posix::size_t(length));
  if(m_data == nullptr) // if no memory allocated yet
  {
    m_data = new_data; // assign memory
    resize(0); // reset all the progress pointers status
  }
  else // expand memory
  {
    std::memcpy(new_data, m_data, posix::size_t(capacity())); // copy old memory to new
    m_virt_begin = new_data + (m_virt_begin - m_data); // shift the progress pointers to the new memory
    m_virt_end   = new_data + (m_virt_end   - m_data);
    delete[] m_data; // delete the old memory
    m_data = new_data; // use new memory
  }

  m_capacity = length;
  m_ok &= m_data != nullptr;
  return m_data != nullptr;
}

void vfifo::reset(void) noexcept
{
  m_virt_end = m_virt_begin = m_data;
  clearError();
}

bool vfifo::resize(posix::ssize_t sz) noexcept
{
  if(sz < 0)
    return m_ok = false;
  m_virt_begin = m_data;
  m_virt_end   = m_data + sz;
  m_ok &= m_virt_end <= end();
  return m_virt_end <= end();
}

bool vfifo::shrink(posix::ssize_t count) noexcept
{
  if(count < 0)
    return m_ok = false;
  if(m_virt_begin + count < m_virt_end)
    m_virt_begin += count;
  else
    m_virt_begin = m_virt_end = m_data; // move back to start of buffer
  return true;
}

bool vfifo::expand(posix::ssize_t count) noexcept
{
  if(count < 0)
    return m_ok = false;
  if(m_virt_end + count < end())
    m_virt_end += count;
  else
    m_virt_end = const_cast<char*>(end());
  return true;
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
