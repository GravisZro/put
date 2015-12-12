#ifndef SERIALIZERS_H
#define SERIALIZERS_H

#include <vector>
#include <string>
#include <cstring>

namespace rpc
{
// ===== interface declarations =====
  template<typename... ArgTypes>
  static std::vector<char> serialize(ArgTypes&... args);

  template<typename... ArgTypes>
  static bool deserialize(std::vector<char>& data, ArgTypes&... args);
// ==================================


// virtual queue class
  class vqueue : public std::vector<char>
  {
  public:
    inline vqueue(void) : m_front(begin()) { reserve(0x2000); }

    inline vqueue(std::vector<char>& data)
      : std::vector<char>(data), m_front(begin()) { }

    inline void push(const char& d) { std::vector<char>::push_back(d); }

    const char& pop(void)
    {
      const char& d = *m_front;
      if(m_front != std::vector<char>::end())
        ++m_front;
      return d;
    }

    inline const char& front(void) const { return *m_front; }
  private:
    std::vector<char>::iterator m_front;
  };

// serializer definitions

  // sized char array
  template<typename T>
  static inline void serialize(vqueue& data, const T* arg, size_t length)
  {
    for(size_t i = 0; i < length * sizeof(T); ++i)
      data.push(reinterpret_cast<const char*>(arg)[i]);
  }

  template<typename T>
  static inline void deserialize(vqueue& data, T* arg, size_t length)
  {
    for(size_t i = 0; i < length * sizeof(T); ++i)
      reinterpret_cast<char*>(arg)[i] = data.pop();
  }

  template<typename T>
  static inline void deserialize(vqueue& data, const T* arg, size_t length)
    { deserialize<T>(data, const_cast<T*>(arg), length); }

  // simple types
  template<typename T>
  static inline void serialize(vqueue& data, T& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");

    data.push(sizeof(arg));
    serialize<T>(data, &arg, 1);
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, T& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");

    if(data.pop() != sizeof(T))
      return false;
    deserialize<T>(data, &arg, 1);
    return true;
  }

  // vector of simple types
  template<typename T>
  static inline void serialize(vqueue& data, const std::vector<T>& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");

    data.push(sizeof(T));
    data.push(arg.size());

    serialize(data, arg.data(), arg.size());
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, std::vector<T>& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");

    if(data.pop() != sizeof(T))
      return false;

    arg.resize(data.front());
    if(arg.size() != data.pop())
      return false;

    deserialize(data, arg.data(), arg.size());
    return true;
  }

  // string
  template<>
  inline void serialize<std::string>(vqueue& data, std::string& arg)
  {
    data.push(arg.size());
    serialize(data, arg.data(), arg.size());
  }

  template<>
  inline bool deserialize<std::string>(vqueue& data, std::string& arg)
  {
    arg.resize(data.front(), 0);
    if(arg.size() != static_cast<unsigned>(data.pop()))
      return false;

    deserialize(data, arg.data(), arg.size());
    return true;
  }

  // wide string
  template<>
  inline void serialize<std::wstring>(vqueue& data, std::wstring& arg)
  {
    data.push(arg.size());
    serialize(data, arg.data(), arg.size());
  }

  template<>
  inline bool deserialize<std::wstring>(vqueue& data, std::wstring& arg)
  {
    arg.resize(data.front(), 0);
    if(arg.size() != static_cast<unsigned>(data.pop()))
      return false;

    deserialize(data, arg.data(), arg.size());
    return true;
  }


  // multi-arg
  template<typename T, typename... ArgTypes>
  static inline void serialize(vqueue& data, T& arg, ArgTypes&... others)
  {
    serialize<T>(data, arg);
    serialize<ArgTypes...>(data, others...);
  }

  template<typename T, typename... ArgTypes>
  static inline bool deserialize(vqueue& data, T& arg, ArgTypes&... others)
  {
    return deserialize<T>(data, arg) &&
           deserialize<ArgTypes...>(data, others...);
  }

  // multi-arg presentation
  template<typename... ArgTypes>
  static inline std::vector<char> serialize(ArgTypes&... args)
  {
    vqueue d;
    serialize(d, args...);
    return d;
  }

  template<typename... ArgTypes>
  static inline bool deserialize(std::vector<char>& data, ArgTypes&... args)
  {
    vqueue d(data);
    return deserialize(d, args...);
  }
}

#endif // SERIALIZERS_H
