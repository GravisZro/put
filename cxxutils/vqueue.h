#ifndef VQUEUE_H
#define VQUEUE_H

// STL
#include <cstdint>
#include <cassert>
#include <cstring>

// virtual queue class
class vqueue
{
public:
  inline vqueue(uint16_t length = 0xFFFF) { allocate(length); }
  inline ~vqueue(void) { deallocate(); }

  vqueue(const vqueue& that) : m_data(nullptr) { operator=(that); }
  vqueue& operator=(const vqueue& other)
  {
    vqueue& that = const_cast<vqueue&>(other);
    deallocate();
    if (this != &that)
    {
      m_data            = that.m_data;
      m_virt_begin      = that.m_virt_begin;
      m_virt_end        = that.m_virt_end;
      m_capacity        = that.m_capacity;
      that.m_data       = nullptr;
      that.m_virt_begin = nullptr;
      that.m_virt_end   = nullptr;
      that.m_capacity   = 0;
    }
    return *this;
  }

  inline void allocate(uint16_t length = 0xFFFF)
  {
    if(length)
      m_data = new char[length];
    else
      m_data = nullptr;
    m_capacity = length;
    resize(0);
  }

  inline void deallocate(void)
  {
    if(m_data != nullptr)
    {
      m_capacity = 0;
      delete m_data;
      m_data = nullptr;
    }
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
    assert(data() <= dataEnd());
    return d;
  }

  inline bool     empty(void) const { return dataEnd() == data(); }
  inline uint16_t size (void) const { return dataEnd() -  data(); }

  inline void resize(uint16_t sz)
  {
    m_virt_begin = m_data;
    m_virt_end   = m_data + sz;
  }


  template<typename T = char> inline const T& front   (void) const { return *data<T>(); }
  template<typename T = char> inline       T& front   (void)       { return *data<T>(); }

  template<typename T = char> inline const T& back    (void) const { return *dataEnd<T>(); }
  template<typename T = char> inline       T& back    (void)       { return *dataEnd<T>(); }

  template<typename T = char> inline const T* data    (void) const { return reinterpret_cast<const T*>(m_virt_begin); }
  template<typename T = char> inline       T* data    (void)       { return reinterpret_cast<      T*>(m_virt_begin); }
  template<typename T = char> inline const T* dataEnd (void) const { return reinterpret_cast<const T*>(m_virt_end  ); }
  template<typename T = char> inline       T* dataEnd (void)       { return reinterpret_cast<      T*>(m_virt_end  ); }

  template<typename T = char> inline const T* begin   (void) const { return reinterpret_cast<const T*>(m_data); }
  template<typename T = char> inline const T* end     (void) const { return begin<T>() + m_capacity; }

  inline const uint16_t& capacity(void) const { return m_capacity; }
private:
  char* m_data;
  char* m_virt_begin;
  char* m_virt_end;
  uint16_t m_capacity;
};

#endif // VQUEUE_H

