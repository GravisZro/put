#ifndef VQUEUE_H
#define VQUEUE_H

// POSIX++
#include <cwchar>

// STL
#include <vector>
#include <string>
#include <utility>

// PUT
#include <put/cxxutils/posix_helpers.h>

#if __GNUC__ >= 7 || __clang_major__ >= 3
# define constexpr_maybe constexpr
#else
# define constexpr_maybe inline
#endif

// virtual queue class
class vfifo
{
public:
  vfifo(posix::ssize_t length = 0x0000FFFF) noexcept;  // default size is 64 KiB
  vfifo(const vfifo& other) noexcept;
  ~vfifo(void) noexcept;

  vfifo& operator=(const vfifo& other) = delete;

// === error functions ===
  constexpr_maybe bool hadError(void) const noexcept { return !m_ok; }
  constexpr_maybe void clearError(void) noexcept { m_ok = true; }

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

  constexpr_maybe bool           empty    (void) const noexcept { return !size(); }
  constexpr_maybe posix::ssize_t size     (void) const noexcept { return m_virt_end - m_virt_begin; }
  constexpr_maybe posix::ssize_t discarded(void) const noexcept { return m_virt_begin; }
  constexpr_maybe posix::ssize_t used     (void) const noexcept { return m_virt_end; }
  constexpr_maybe posix::ssize_t unused   (void) const noexcept { return m_capacity - m_virt_end; }
  constexpr_maybe posix::ssize_t capacity (void) const noexcept { return m_capacity; }

  bool allocate(posix::ssize_t length) noexcept;
  void reset(void) noexcept;

  template<typename T = char> constexpr T& front   (void) noexcept       { return *data<T>(); }
  template<typename T = char> constexpr T& back    (void) noexcept       { return *dataEnd<T>(); }

  template<typename T = char> constexpr T* data    (void) const noexcept { return reinterpret_cast<T*>(static_cast<uint8_t*>(m_data) + m_virt_begin); }
  template<typename T = char> constexpr T* dataEnd (void) const noexcept { return reinterpret_cast<T*>(static_cast<uint8_t*>(m_data) + m_virt_end  ); }

  template<typename T = char> constexpr T* begin   (void) const noexcept { return reinterpret_cast<T*>(m_data); }
  template<typename T = char> constexpr T* end     (void) const noexcept { return reinterpret_cast<T*>(static_cast<uint8_t*>(m_data) + m_capacity); }

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
    if(empty()) // if buffer is empty
      reset(); // move back to start of buffer
    return true;
  }

  void* m_data;
  posix::ssize_t m_virt_begin;
  posix::ssize_t m_virt_end;
  posix::ssize_t m_capacity;
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
    if(size() < discarded())
      allocate(m_capacity);
    if(posix::ssize_t(sizeof(T)) > unused())
      allocate(m_capacity * 2);

    if(push<uint16_t>(sizeof(T)) &&
       push<uint16_t>(length))
      for(posix::size_t i = 0; m_ok && i < length; ++i)
        push<T>(arg[i]);
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
    { serialize_arr(arg, uint16_t(posix::strlen(arg))); }

  void serialize(const wchar_t* arg) noexcept
    { serialize_arr(arg, uint16_t(std::wcslen(arg))); }

// string
  template<typename T>
  void deserialize(std::basic_string<T>& arg) noexcept
  {
    arg.resize(data<uint16_t>()[1]);
    deserialize_arr(const_cast<T*>(arg.data()), arg.size()); // not guaranteed to work per the STL spec but should never fail catastrophically.
  }
};

// make sure strings are read in properly!
template<> vfifo& vfifo::operator << (const std::string& arg) noexcept;
template<> vfifo& vfifo::operator << (const std::wstring& arg) noexcept;
#endif // VQUEUE_H
