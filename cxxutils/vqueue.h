#ifndef VQUEUE_H
#define VQUEUE_H

// STL
#include <cstdint>
#include <memory>

// virtual queue class
class vqueue
{
public:
  inline vqueue(uint16_t length = 0xFFFF) { allocate(length); }
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
    }
    return *this;
  }

  inline bool allocate(uint16_t length = 0xFFFF)
  {
    m_data.reset(new char[length]);
    m_capacity = length;
    resize(0);
    return m_data.get() != nullptr;
  }

  template<typename T>
  inline bool push(const T& d)
  {
    if(dataEnd<T>() + 1 >= end<T>())
      return false;
    back<T>() = d;
    m_virt_end += sizeof(T);
    return true;
  }

  template<typename T = char>
  bool pop(void)
  {
    if(data<T>() + 1 > dataEnd<T>())
      return false;
    m_virt_begin += sizeof(T);
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
      return false;
    if(m_virt_begin + count < m_virt_end)
      m_virt_begin += count;
    else
      m_virt_begin = m_virt_end;
    return true;
  }

  inline bool expand(ssize_t count)
  {
    if(count < 0)
      return false;
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
};

#endif // VQUEUE_H

