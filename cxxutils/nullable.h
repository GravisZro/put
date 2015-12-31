#ifndef NULLABLE_H
#define NULLABLE_H

// STL
#include <cstddef>
#include <ostream>

template <typename T>
class nullable final
{
public:
  inline nullable(void) : m_isnull(true), m_value(T()) { }
  inline nullable(const T& value) : m_isnull(false), m_value(value) { }
  inline nullable(std::nullptr_t) : m_isnull(true), m_value(T()) { }

  inline const nullable<T>& operator=(const nullable<T>& value) { m_isnull = value.m_isnull; m_value = value.m_value; return *this; }
  inline const nullable<T>& operator=(const T& value) { m_isnull = false; m_value = value; return *this; }
  inline const nullable<T>& operator=(std::nullptr_t) { m_isnull = true; m_value = T(); return *this; }

  inline bool is_null(void) const { return m_isnull; }
  inline const T& value(void) const { return m_value; }

  // operators
  inline operator const T& (void) const { return value(); }

  inline bool operator==(std::nullptr_t) const { return is_null(); }
  inline bool operator!=(std::nullptr_t v) const { return !operator ==(v); }

  inline bool operator==(const T& v) const { return is_null() && value() == v; }
  inline bool operator!=(const T& v) const { return !operator ==(v); }

  inline bool operator==(const nullable<T>& o) const
    { return (is_null() && o.is_null()) || (!is_null() && !o.is_null() && value() == o.value()); }

  inline size_t size(void) const { return m_isnull ? 0 : sizeof(T); }

  // reroute comparisons to class operators
  template <typename T2> inline friend bool operator==(const T2& v, const nullable<T>& o) { return o == v; }
  template <typename T2> inline friend bool operator!=(const T2& v, const nullable<T>& o) { return o != v;  }
private:
  bool m_isnull;
  T m_value;
};

template <typename T>
static std::ostream& operator <<(std::ostream& os, const nullable<T>& v)
{
  if(v.is_null())
    os << "null";
  else
    os << v.value();
  return os;
}

#endif // NULLABLE_H

