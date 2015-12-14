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
    back<T>() = d;
    m_virt_end += sizeof(T);
    assert(dataEnd() < end());
  }

  template<typename T = char>
  const T& pop(void)
  {
    const T& d = front<T>();
    m_virt_begin += sizeof(T);
    assert(dataBegin() <= dataEnd());
    return d;
  }

  inline uint16_t size(void) const { return dataEnd() - dataBegin(); }

  inline void resize(uint16_t sz)
  {
    m_virt_begin = m_data;
    m_virt_end   = m_data + sz;
  }


  template<typename T = char> inline const T& front     (void) const { return *dataBegin<T>(); }
  template<typename T = char> inline       T& front     (void)       { return *dataBegin<T>(); }

  template<typename T = char> inline const T& back      (void) const { return *dataEnd<T>(); }
  template<typename T = char> inline       T& back      (void)       { return *dataEnd<T>(); }

  template<typename T = char> inline const T* dataBegin (void) const { return reinterpret_cast<const T*>(m_virt_begin); }
  template<typename T = char> inline       T* dataBegin (void)       { return reinterpret_cast<      T*>(m_virt_begin); }
  template<typename T = char> inline const T* dataEnd   (void) const { return reinterpret_cast<const T*>(m_virt_end  ); }
  template<typename T = char> inline       T* dataEnd   (void)       { return reinterpret_cast<      T*>(m_virt_end  ); }

  template<typename T = char> inline const T* begin     (void) const { return reinterpret_cast<const T*>(m_data); }
  template<typename T = char> inline const T* end       (void) const { return begin<T>() + sizeof(m_data); }

  inline uint16_t capacity(void) const { return sizeof(m_data); }
private:
  char m_data[0xFFFF]; // assumed to be contiguous
  char* m_virt_begin;
  char* m_virt_end;
};
#endif // VQUEUE_H

