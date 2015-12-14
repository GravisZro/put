#ifndef VQUEUE_H
#define VQUEUE_H

// STL
#include <cstdint>
#include <cassert>
#include <cstring>
#include <vector>

// virtual queue class
class vqueue
{
public:
  inline vqueue(void) { resize(0); }
  inline vqueue(std::vector<uint8_t>& data)
  {
    assert(data.size() < capacity());
    resize(data.size());
    std::memcpy(m_data, data.data(), size());
  }

  template<typename T>
  inline void push(const T& d)
  {
    *reinterpret_cast<T*>(m_virt_end) = d;
    m_virt_end += sizeof(T);
    assert(m_virt_end < end());
  }

  template<typename T>
  const T& pop(void)
  {
    const T& d = front<T>();
    m_virt_begin += sizeof(T);
    assert(m_virt_end < end());
    return d;
  }

  template<typename T>
  inline const T& front(void) const
    { return *reinterpret_cast<T*>(m_virt_begin); }

  template<typename T>
  inline const T& back (void) const
    { return *reinterpret_cast<T*>(m_virt_end); }


  inline uint16_t size(void) const { return m_virt_end - data(); }

  inline void resize(uint16_t sz)
  {
    m_virt_begin = m_data;
    m_virt_end   = m_data + sz;
  }

  inline uint16_t capacity(void) const { return sizeof(m_data); }
  inline const uint8_t* data (void) const { return m_data; }
  inline const uint8_t* end  (void) const { return data() + sizeof(m_data); }
private:
  uint8_t m_data[0xFFFF]; // assumed to be contiguous
  uint8_t* m_virt_begin;
  uint8_t* m_virt_end;
};
#endif // VQUEUE_H

