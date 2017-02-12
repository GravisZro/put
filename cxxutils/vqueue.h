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

// virtual queue class
class vqueue
{
public:
  vqueue(uint16_t length = 0xFFFF) noexcept : m_ok(true) { allocate(length); }
  vqueue(const vqueue& that) noexcept { operator=(that); }

  vqueue& operator=(const vqueue& other) noexcept
  {
    if (this != &other)
    {
      m_data       = const_cast<vqueue&>(other).m_data;
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
  constexpr vqueue& operator << (const T& arg) noexcept
  {
    serialize(arg);
    return *this;
  }

  template<typename T>
  constexpr vqueue& operator >> (T& arg) noexcept
  {
    deserialize(arg);
    return *this;
  }

  template<typename T, typename... ArgTypes>
  constexpr void serialize(const T& arg, ArgTypes&... args) noexcept
  {
    serialize(arg);
    serialize(args...);
  }

  template<typename T, typename... ArgTypes>
  constexpr void deserialize(T& arg, ArgTypes&... args) noexcept
  {
    deserialize(arg);
    deserialize(args...);
  }


// === manual queue manipulators ===
  bool allocate(uint16_t length = 0xFFFF) noexcept
  {
    m_data.reset(new char[length]);
    memset(m_data.get(), 0, length);

    m_capacity = length;
    resize(0);
    m_ok &= m_data.get() != nullptr;
    return m_data.get() != nullptr;
  }

  template<typename T>
  bool push(const T& d) noexcept
  {
    if(dataEnd<T>() + 1 > end<T>())
      return m_ok = false;
    back<T>() = d;
    m_virt_end += sizeof(T);
    return true;
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


  bool resize(uint16_t sz) noexcept
  {
    m_virt_begin = m_data.get();
    m_virt_end   = m_data.get() + sz;
    return m_virt_end <= end();
  }

  bool shrink(ssize_t count) noexcept
  {
    if(count < 0)
      return m_ok = false;
    if(m_virt_begin + count < m_virt_end)
      m_virt_begin += count;
    else
      m_virt_begin = m_virt_end = m_data.get(); // move back to start of buffer
    return true;
  }

  bool expand(ssize_t count) noexcept
  {
    if(count < 0)
      return m_ok = false;
    if(m_virt_end + count < end())
      m_virt_end += count;
    else
      m_virt_end = const_cast<char*>(end());
    return true;
  }

  //template<typename T = char> constexpr const T& front   (void) const { return *data<T>(); }
  template<typename T = char> constexpr       T& front   (void) noexcept       { return *data<T>(); }

  //template<typename T = char> constexpr const T& back    (void) const { return *dataEnd<T>(); }
  template<typename T = char> constexpr       T& back    (void) noexcept       { return *dataEnd<T>(); }

  template<typename T = char> constexpr       T* data    (void) const noexcept { return reinterpret_cast<T*>(m_virt_begin); }
  template<typename T = char> constexpr       T* dataEnd (void) const noexcept { return reinterpret_cast<T*>(m_virt_end  ); }

  template<typename T = char>                 T* begin   (void) const noexcept { return reinterpret_cast<T*>(m_data.get()); }
  template<typename T = char> constexpr       T* end     (void) const noexcept { return reinterpret_cast<T*>(m_data.get() + m_capacity); }

  const uint16_t& capacity(void) const { return m_capacity; }

private:
  template<typename T = char>
  bool pop(void) noexcept
  {
    if(data<T>() + 1 > dataEnd<T>())
      return m_ok = false;
    m_virt_begin += sizeof(T);
    if(m_virt_begin == m_virt_end) // if buffer is empty
      m_virt_begin = m_virt_end = m_data.get(); // move back to start of buffer
    return true;
  }

  std::shared_ptr<char> m_data;
  char* m_virt_begin;
  char* m_virt_end;
  uint16_t m_capacity;
  bool m_ok;

// === serializer backends ===
private:
  // dummy functions
  static inline void serialize(void) { }
  static inline void deserialize(void) { }

// sized array
  template<typename T>
  void serialize_arr(const T* arg, uint16_t length) noexcept
  {
    if(push<uint16_t>(sizeof(T)) &&
       push<uint16_t>(length))
      for(size_t i = 0; m_ok && i < length; push(arg[i]), ++i);
  }

  template<typename T>
  void deserialize_arr(T* arg, uint16_t length) noexcept
  {
    if(front<uint16_t>() == sizeof(T) &&  // size matches
       pop  <uint16_t>() &&               // no buffer underflow
       front<uint16_t>() == length &&     // length matches
       pop  <uint16_t>())                 // no buffer underflow

      for(size_t i = 0; m_ok && i < length; pop<T>(), ++i)
        arg[i] = front<T>();
    else
      m_ok = false;
  }

// simple types
  template<typename T>
  constexpr void serialize(const T& arg) noexcept
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "compound or pointer type");
    serialize_arr<T>(&arg, 1);
  }

  template<typename T>
  constexpr void deserialize(T& arg) noexcept
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "compound or pointer type");
    deserialize_arr<T>(&arg, 1);
  }

// vector of simple types
  template<typename T>
  constexpr void serialize(const std::vector<T>& arg) noexcept
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "vector of compound or pointer type");
    serialize_arr(arg.data(), arg.size());
  }

  template<typename T>
  constexpr void deserialize(std::vector<T>& arg) noexcept
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "vector of compound or pointer type");
    arg.resize(front<uint16_t>());
    deserialize_arr(arg.data(), arg.size());
  }

// string literals
  void serialize(const char* arg) noexcept
    { serialize_arr(arg, std::strlen(arg)); }

  void serialize(const wchar_t* arg) noexcept
    { serialize_arr(arg, std::wcslen(arg)); }

// string
  template<typename T>
  constexpr void serialize(const std::basic_string<T>& arg) noexcept
    { serialize_arr(arg.data(), arg.size()); }

  template<typename T>
  constexpr void deserialize(std::basic_string<T>& arg) noexcept
  {
    arg.resize(front<uint16_t>());
    deserialize_arr(const_cast<T*>(arg.data()), arg.size()); // not guaranteed to work per the STL spec but will never fail catastrophically.
  }
};

#endif // VQUEUE_H

