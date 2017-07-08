#ifndef VQUEUE_H
#define VQUEUE_H

// C++
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cwchar>

// STL
#include <memory>
#include <vector>
#include <string>
#include <utility>

// PDTK
#include <cxxutils/posix_helpers.h>

// virtual queue class
class vfifo
{
public:
  vfifo(posix::ssize_t length = 0x0000FFFF) noexcept  // default size is 64 KiB
    : m_ok(true) { allocate(length); }
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
  bool allocate(posix::size_t length = 0x0000FFFF) noexcept // 64 KiB
  {
    m_data.reset(new char[length]);
    std::memset(m_data.get(), 0, length);

    m_capacity = length;
    resize(0);
    m_ok &= m_data.get() != nullptr;
    return m_data.get() != nullptr;
  }

  bool     empty (void) const noexcept { return dataEnd() == data   (); }
  uint16_t size  (void) const noexcept { return dataEnd() -  data   (); }
  uint16_t used  (void) const noexcept { return data   () -  begin  (); }
  uint16_t unused(void) const noexcept { return end    () -  dataEnd(); }


  void reset(void) noexcept
  {
    m_virt_end = m_virt_begin = m_data.get();
    clearError();
  }

  bool resize(posix::size_t sz) noexcept
  {
    m_virt_begin = m_data.get();
    m_virt_end   = m_data.get() + sz;
    m_ok &= m_virt_end <= end();
    return m_virt_end <= end();
  }

  bool shrink(posix::size_t count) noexcept
  {
    if(m_virt_begin + count < m_virt_end)
      m_virt_begin += count;
    else
      m_virt_begin = m_virt_end = m_data.get(); // move back to start of buffer
    return true;
  }

  bool expand(posix::size_t count) noexcept
  {
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

  template<typename T = char>           T* begin   (void) const noexcept { return reinterpret_cast<T*>(m_data.get()); }
  template<typename T = char> constexpr T* end     (void) const noexcept { return reinterpret_cast<T*>(m_data.get() + m_capacity); }

  const posix::size_t& capacity(void) const { return m_capacity; }

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
      m_virt_begin = m_virt_end = m_data.get(); // move back to start of buffer
    return true;
  }

  std::shared_ptr<char> m_data;
  char* m_virt_begin;
  char* m_virt_end;
  posix::size_t m_capacity;
  bool m_ok;

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
      for(uint16_t i = front<uint16_t>(); pull<V>() && i > 0; --i)
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
    { serialize_arr(arg, std::strlen(arg)); }

  void serialize(const wchar_t* arg) noexcept
    { serialize_arr(arg, std::wcslen(arg)); }

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
