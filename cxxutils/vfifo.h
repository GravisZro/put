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
class vfifo
{
public:
  vfifo(uint16_t length = 0xFFFF) noexcept : m_ok(true) { allocate(length); }
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
  bool allocate(uint16_t length = 0xFFFF) noexcept
  {
    m_data.reset(new char[length]);
    memset(m_data.get(), 0, length);

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

  bool resize(uint16_t sz) noexcept // causing this to fail would be exceptionally difficult
  {
    m_virt_begin = m_data.get();
    m_virt_end   = m_data.get() + sz;
    m_ok &= m_virt_end <= end();
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
  uint16_t m_capacity;
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
      for(size_t i = 0; m_ok && i < length; ++i)
        push(arg[i]);
  }

  template<typename T>
  void deserialize_arr(T* arg, uint16_t length) noexcept
  {
    if(front<uint16_t>() == sizeof(T) &&  // size matches
       pull  <uint16_t>() &&               // no buffer underflow
       front<uint16_t>() == length &&     // length matches
       pull  <uint16_t>())                 // no buffer underflow
      for(size_t i = 0; m_ok && i < length; ++i)
      {
        arg[i] = front<T>();
        pull<T>();
      }
    else
      m_ok = false;
  }

// simple types
  template<typename T>
  void serialize(const T& arg) noexcept
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "compound or pointer type");
    serialize_arr<T>(&arg, 1);
  }

  template<typename T>
  void deserialize(T& arg) noexcept
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "compound or pointer type");
    deserialize_arr<T>(&arg, 1);
  }

// vector of simple types
  template<typename T>
  void serialize(const std::vector<T>& arg) noexcept
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "vector of compound or pointer type");
    serialize_arr(arg.data(), arg.size());
  }

  template<typename T>
  void deserialize(std::vector<T>& arg) noexcept
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "vector of compound or pointer type");
    arg.resize(data<uint16_t>()[1]);
    deserialize_arr(arg.data(), arg.size());
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
