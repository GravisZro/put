#ifndef RANGED_H
#define RANGED_H

template<typename T, T min, T max>
class ranged
{
public:
  constexpr ranged(void) : m_value(max + 1) { }
  constexpr bool isValid(void) { return m_value >= min && m_value <= max; }
  constexpr operator T&(void) { return m_value; }
  constexpr auto& operator =(T value) { m_value = value; return *this; }
private:
  T m_value;
};

#endif // RANGED_H
