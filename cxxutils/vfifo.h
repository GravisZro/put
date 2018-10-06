#ifndef VQUEUE_H
#define VQUEUE_H

// POSIX++
#include <cstdint>
#include <cstring>
#include <cwchar>

// STL
#include <vector>
#include <string>
#include <utility>
#include <new>

// PDTK
#include <cxxutils/posix_helpers.h>

// virtual queue class
class vfifo
{
public:
  template<typename... ArgTypes>
  vfifo(ArgTypes&... args) noexcept : vfifo() { serialize(args...); }

  vfifo(char* data, posix::ssize_t length) noexcept
    : m_data(data), m_capacity(length), m_external(true) { reset(); }

  vfifo(posix::ssize_t length = 0x0000FFFF) noexcept  // default size is 64 KiB
    : m_data(nullptr), m_capacity(0), m_external(false) { allocate(length); }

  vfifo(const vfifo& that) noexcept { operator=(that); }

  vfifo& operator=(const vfifo& other) noexcept
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

// === error functions ===
  bool hadError(void) const noexcept { return !m_ok; }
  void clearError(void) noexcept { m_ok = true; }

// === serializer frontends ===
  template<typename T>
  vfifo& operator << (const T& arg) noexcept
  {
    if(posix::ssize_t(sizeof(T)) > unused())
      allocate(capacity() * 2);
    serialize(arg);
    return *this;
  }

  template<typename T>
  vfifo& operator >> (T& arg) noexcept
  {
    deserialize(arg);
    return *this;
  }

  template<typename T, typename... ArgTypes>
  void serialize(const T& arg, ArgTypes&... args) noexcept
  {
    if(posix::ssize_t(sizeof(T)) > unused())
      allocate(capacity() * 2);
    serialize(arg);
    serialize(args...);
  }

  template<typename T, typename... ArgTypes>
  void deserialize(T& arg, ArgTypes&... args) noexcept
  {
    deserialize(arg);
    deserialize(args...);
  }

// === manual queue manipulators ===
  bool allocate(posix::ssize_t length) noexcept
  {
    if(m_external)
      return m_ok = false;

    if(length <= capacity()) // reducing the allocated size is not allowed!
      return false;
    char* new_data = new(std::nothrow) char[posix::size_t(length)];
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

  bool           empty (void) const noexcept { return dataEnd() == data   (); }
  posix::ssize_t size  (void) const noexcept { return dataEnd() -  data   (); }
  posix::ssize_t used  (void) const noexcept { return data   () -  begin  (); }
  posix::ssize_t unused(void) const noexcept { return end    () -  dataEnd(); }


  void reset(void) noexcept
  {
    m_virt_end = m_virt_begin = m_data;
    clearError();
  }

  bool resize(posix::ssize_t sz) noexcept
  {
    if(sz < 0)
      return m_ok = false;
    m_virt_begin = m_data;
    m_virt_end   = m_data + sz;
    m_ok &= m_virt_end <= end();
    return m_virt_end <= end();
  }

  bool shrink(posix::ssize_t count) noexcept
  {
    if(count < 0)
      return m_ok = false;
    if(m_virt_begin + count < m_virt_end)
      m_virt_begin += count;
    else
      m_virt_begin = m_virt_end = m_data; // move back to start of buffer
    return true;
  }

  bool expand(posix::ssize_t count) noexcept
  {
    if(count < 0)
      return m_ok = false;
    if(m_virt_end + count < end())
      m_virt_end += count;
    else
      m_virt_end = const_cast<char*>(end());
    return true;
  }

  template<typename T = char> constexpr T& front   (void) noexcept       { return *data<T>(); }
  template<typename T = char> constexpr T& back    (void) noexcept       { return *dataEnd<T>(); }

  template<typename T = char> constexpr T* data    (void) const noexcept { return reinterpret_cast<T*>(m_virt_begin); }
  template<typename T = char> constexpr T* dataEnd (void) const noexcept { return reinterpret_cast<T*>(m_virt_end  ); }

  template<typename T = char>           T* begin   (void) const noexcept { return reinterpret_cast<T*>(m_data); }
  template<typename T = char> constexpr T* end     (void) const noexcept { return reinterpret_cast<T*>(m_data + m_capacity); }

  const posix::ssize_t& capacity(void) const { return m_capacity; }

private:
  template<typename T>
  bool push(const T& d) noexcept
  {
    if(dataEnd() + sizeof(T) > end()) // check if this would overflow buffer
      return m_ok = false; // avoid buffer overflow!
    back<T>() = d;
    m_virt_end += sizeof(T);
    return true;
  }

  template<typename T>
  bool pull(void) noexcept
  {
    if(data() + sizeof(T) > dataEnd()) // check if this would underflow buffer
      return m_ok = false; // avoid buffer underflow!
    m_virt_begin += sizeof(T);
    if(m_virt_begin == m_virt_end) // if buffer is empty
      m_virt_begin = m_virt_end = m_data; // move back to start of buffer
    return true;
  }

  char* m_data;
  char* m_virt_begin;
  char* m_virt_end;
  posix::ssize_t m_capacity;
  bool m_ok;
  bool m_external;

// === serializer backends ===
private:
  // dummy functions
  static void serialize(void) noexcept { }
  static void deserialize(void) noexcept { }

// sized array
  template<typename T>
  void serialize_arr(const T* arg, uint16_t length) noexcept
  {
    if(push<uint16_t>(sizeof(T)) &&
       push<uint16_t>(length))
      for(posix::size_t i = 0; m_ok && i < length; ++i)
        push(arg[i]);
  }

  template<typename T>
  void deserialize_arr(T* arg, uint16_t length) noexcept
  {
    if(front<uint16_t>() == sizeof(T) &&  // size matches
       pull <uint16_t>() &&               // no buffer underflow
       front<uint16_t>() == length &&     // length matches
       pull <uint16_t>())                 // no buffer underflow
      for(posix::size_t i = 0; m_ok && i < length; ++i, pull<T>())
        arg[i] = front<T>();
  }

// vector helpers
  template<typename T>
  void serialize_arr(const std::vector<T>& arg) noexcept
    { serialize_arr(arg.data(), arg.size()); }

  template<typename T>
  void deserialize_arr(std::vector<T>& arg) noexcept
  {
    arg.resize(data<uint16_t>()[1]);
    deserialize_arr(const_cast<T*>(arg.data()), arg.size()); // not guaranteed to work per the STL spec but should never fail catastrophically.
  }

// simple types
  template<typename T>
  void serialize(const T arg) noexcept
    { serialize_arr(&arg, 1); }

  template<typename T>
  void deserialize(T& arg) noexcept
    { deserialize_arr(&arg, 1); }


// multi-element STL containers
  template<template<class> class T, class V>
  void serialize(const T<V>& arg) noexcept
  {
    if(std::is_integral<V>::value || std::is_floating_point<V>::value) // if contains a simple type
      serialize_arr(arg);
    else if(push<uint16_t>(UINT16_MAX) && // complex element type
            push<uint16_t>(arg.size()))
      for(const auto& element : arg) // NOTE: this is the most compatible way to iterate an STL container
        if(m_ok)
          serialize(element);
  }

  template<template<class> class T, class V>
  void deserialize(T<V>& arg) noexcept
  {
    if(std::is_integral<V>::value || std::is_floating_point<V>::value) // if contains a simple type
      deserialize_arr(arg);
    else if(front<uint16_t>() == UINT16_MAX && // complex match
            pull <uint16_t>())                 // no buffer underflow
    {
      V tmp;
      for(uint16_t i = front<uint16_t>(); pull<V>() && i > 0; --i) // note: will skip empty containers
      {
        deserialize(tmp);
        arg.emplace(tmp); // NOTE: this is the most compatible way to add element to an STL container
      }
    }
  }

// pair wrapper
  template<typename T, typename V>
  void serialize(const std::pair<T, V>& arg) noexcept
  {
    if(push<uint16_t>(UINT16_MAX) && // complex element type
       push<uint16_t>(2))
    {
      serialize(arg.first);
      serialize(arg.second);
    }
  }

  template<typename T, typename V>
  void deserialize(std::pair<T, V>& arg) noexcept
  {
    if(front<uint16_t>() == UINT16_MAX && // complex match
       pull <uint16_t>() &&               // no buffer underflow
       front<uint16_t>() == 2 &&          // pair length
       pull <uint16_t>())
    {
      deserialize(arg.first);
      deserialize(arg.second);
    }
  }

// string literals
  void serialize(const char* arg) noexcept
    { serialize_arr(arg, uint16_t(std::strlen(arg))); }

  void serialize(const wchar_t* arg) noexcept
    { serialize_arr(arg, uint16_t(std::wcslen(arg))); }

// string
  template<typename T>
  void serialize(const std::basic_string<T>& arg) noexcept
    { serialize_arr(arg.data(), arg.size()); }

  template<typename T>
  void deserialize(std::basic_string<T>& arg) noexcept
  {
    arg.resize(data<uint16_t>()[1]);
    deserialize_arr(const_cast<T*>(arg.data()), arg.size()); // not guaranteed to work per the STL spec but should never fail catastrophically.
  }
};
#endif // VQUEUE_H
