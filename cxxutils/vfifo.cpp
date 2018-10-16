#include "vfifo.h"

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
