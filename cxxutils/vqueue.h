#ifndef VQUEUE_H
#define VQUEUE_H

// STL
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cwchar>
#include <memory>
#include <vector>
#include <string>

// virtual queue class
class vqueue
{
public:
  inline vqueue(uint16_t length = 0xFFFF) : m_ok(true) { allocate(length); }
  vqueue(const vqueue& that) { operator=(that); }

  vqueue& operator=(const vqueue& other)
  {
    vqueue& that = const_cast<vqueue&>(other);
    if (this != &that)
    {
      m_data       = that.m_data;
      m_virt_begin = that.m_virt_begin;
      m_virt_end   = that.m_virt_end;
      m_capacity   = that.m_capacity;
      m_ok         = that.m_ok;
    }
    return *this;
  }

  inline bool hadError(void) const { return !m_ok; }
  inline void clearError(void) { m_ok = true; }

  inline vqueue& operator << (const char* arg)
  {
    serialize_arr(arg, std::strlen(arg));
    return *this;
  }

  inline vqueue& operator << (const wchar_t* arg)
  {
    serialize_arr(arg, std::wcslen(arg));
    return *this;
  }

  template<typename T>
  inline vqueue& operator << (const T& arg)
  {
    serialize(arg);
    return *this;
  }

  template<typename T>
  inline vqueue& operator >> (T& arg)
  {
    deserialize(arg);
    return *this;
  }

  inline bool allocate(uint16_t length = 0xFFFF)
  {
    m_data.reset(new char[length]);
    memset(m_data.get(), 0, length);

    m_capacity = length;
    resize(0);
    m_ok &= m_data.get() != nullptr;
    return m_data.get() != nullptr;
  }

  template<typename T>
  inline bool push(const T& d)
  {
    if(dataEnd<T>() + 1 > end<T>())
      return m_ok = false;
    back<T>() = d;
    m_virt_end += sizeof(T);
    return true;
  }

  template<typename T = char>
  bool pop(void)
  {
    if(data<T>() + 1 > dataEnd<T>())
      return m_ok = false;
    m_virt_begin += sizeof(T);
    if(m_virt_begin == m_virt_end) // if buffer is empty
      m_virt_begin = m_virt_end = m_data.get(); // move back to start of buffer
    return true;
  }

  inline bool     empty (void) const { return dataEnd() == data   (); }
  inline uint16_t size  (void) const { return dataEnd() -  data   (); }
  inline uint16_t used  (void) const { return data   () -  begin  (); }
  inline uint16_t unused(void) const { return end    () -  dataEnd(); }

  inline bool resize(uint16_t sz)
  {
    m_virt_begin = m_data.get();
    m_virt_end   = m_data.get() + sz;
    return m_virt_end <= end();
  }

  inline bool shrink(ssize_t count)
  {
    if(count < 0)
      return m_ok = false;
    if(m_virt_begin + count < m_virt_end)
      m_virt_begin += count;
    else
      m_virt_begin = m_virt_end = m_data.get(); // move back to start of buffer
    return true;
  }

  inline bool expand(ssize_t count)
  {
    if(count < 0)
      return m_ok = false;
    if(m_virt_end + count < end())
      m_virt_end += count;
    else
      m_virt_end = const_cast<char*>(end());
    return true;
  }

  template<typename T = char> inline const T& front   (void) const { return *data<T>(); }
  template<typename T = char> inline       T& front   (void)       { return *data<T>(); }

  template<typename T = char> inline const T& back    (void) const { return *dataEnd<T>(); }
  template<typename T = char> inline       T& back    (void)       { return *dataEnd<T>(); }

  template<typename T = char> inline       T* data    (void) const { return reinterpret_cast<T*>(m_virt_begin); }
  template<typename T = char> inline       T* dataEnd (void) const { return reinterpret_cast<T*>(m_virt_end  ); }

  template<typename T = char> inline       T* begin   (void) const { return reinterpret_cast<T*>(m_data.get()); }
  template<typename T = char> inline       T* end     (void) const { return reinterpret_cast<T*>(m_data.get() + m_capacity); }

  inline const uint16_t& capacity(void) const { return m_capacity; }

private:
  std::shared_ptr<char> m_data;
  char* m_virt_begin;
  char* m_virt_end;
  uint16_t m_capacity;
  bool m_ok;

private:
// sized array
  template<typename T>
  inline void serialize_arr(const T* arg, uint16_t length)
  {
    if(push<uint16_t>(length) &&
       push<uint8_t>(sizeof(T)))
      for(size_t i = 0; m_ok && i < length; push(arg[i]), ++i);
  }

  template<typename T>
  inline void deserialize_arr(T* arg, uint16_t length)
  {
    if(front<uint16_t>() == length &&    // length mismatch
       pop  <uint16_t>() &&              // buffer underflow
       front<uint8_t >() == sizeof(T) && // size mismatch
       pop  <uint8_t >())                // buffer underflow
      for(size_t i = 0; m_ok && i < length; pop<T>(), ++i)
        arg[i] = front<T>();
    else
      m_ok = false;
  }

// simple types
  template<typename T>
  inline void serialize(const T& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "compound or pointer type");
    serialize_arr<T>(&arg, 1);
  }

  template<typename T>
  inline void deserialize(T& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "compound or pointer type");
    deserialize_arr<T>(&arg, 1);
  }

// vector of simple types
  template<typename T>
  inline void serialize(const std::vector<T>& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "vector of compound or pointer type");
    serialize_arr(arg.data(), arg.size());
  }

  template<typename T>
  inline void deserialize(std::vector<T>& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "vector of compound or pointer type");
    arg.resize(front<uint16_t>());
    deserialize_arr(arg.data(), arg.size());
  }

// string
  template<typename T>
  inline void serialize(const std::basic_string<T>& arg)
    { serialize_arr(arg.data(), arg.size()); }

  template<typename T>
  inline void deserialize(std::basic_string<T>& arg)
  {
    arg.resize(front<uint16_t>());
    deserialize_arr(const_cast<T*>(arg.data()), arg.size()); // bad form! not guaranteed to work.
  }
};

#endif // VQUEUE_H

