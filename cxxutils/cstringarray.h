#ifndef CSTRINGARRAY_H
#define CSTRINGARRAY_H

// STL
#include <string>
#include <vector>

#include <ostream>

class CStringArray
{
public:
  template<class container>
  CStringArray(container& data) noexcept
    : CStringArray(data, [](const auto& p) { return p; }) { }

  template<class container, typename filter>
  CStringArray(container& data, filter func) noexcept
  {
    for(auto& pos : data)
      m_datav.push_back(func(pos));
    for(auto& pos : m_datav)
      m_datapv.push_back(pos.c_str());
    m_datapv.push_back(nullptr);
  }

  operator char* const* (void) const noexcept { return (char* const*)m_datapv.data(); }

private:
  std::vector<std::string> m_datav;
  std::vector<const char*> m_datapv;
};


#endif // CSTRINGARRAY_H
