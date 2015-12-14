#ifndef SERIALIZERS_H
#define SERIALIZERS_H

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

namespace rpc
{
// ===== interface declarations =====
  template<typename... ArgTypes>
  static std::vector<uint8_t> serialize(ArgTypes&... args);

  template<typename... ArgTypes>
  static bool deserialize(std::vector<uint8_t>& data, ArgTypes&... args);
// ==================================


// virtual queue class
  class vqueue : public std::vector<uint8_t>
  {
  public:
    inline vqueue(void) : m_front(begin()) { reserve(0xFFFF); }

    inline vqueue(std::vector<uint8_t>& data)
      : std::vector<uint8_t>(data), m_front(begin()) { }

    inline void push(const uint8_t& d) { std::vector<uint8_t>::push_back(d); }

    const uint8_t& pop(void)
    {
      const uint8_t& d = *m_front;
      if(m_front != std::vector<uint8_t>::end())
        ++m_front;
      return d;
    }

    inline const uint8_t& front(void) const { return *m_front; }
  private:
    std::vector<uint8_t>::iterator m_front;
  };

// ===== serializer definitions =====

// sized array
  template<typename T>
  static inline void serialize(vqueue& data, const T* arg, size_t length)
  {
    data.push(length);
    data.push(sizeof(T));
    for(size_t i = 0; i < length * sizeof(T); ++i)
      data.push(reinterpret_cast<const uint8_t*>(arg)[i]);
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, T* arg, uint8_t length)
  {
    if(data.pop() != length)
      return false;

    if(data.pop() != sizeof(T))
      return false;

    for(size_t i = 0; i < length * sizeof(T); ++i)
      reinterpret_cast<uint8_t*>(arg)[i] = data.pop();
    return true;
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, const T* arg, uint8_t length)
    { return deserialize<T>(data, const_cast<T*>(arg), length); }

// simple types
  template<typename T>
  static inline void serialize(vqueue& data, T& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");
    serialize<T>(data, &arg, 1);
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, T& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");
    return deserialize<T>(data, &arg, 1);
  }

// vector of simple types
  template<typename T>
  static inline void serialize(vqueue& data, const std::vector<T>& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");
    serialize(data, arg.data(), arg.size());
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, std::vector<T>& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");
    arg.resize(data.front());
    return deserialize(data, arg.data(), arg.size());
  }

// const char* string
  template<>
  inline void serialize<const char*>(vqueue& data, const char*& arg)
    { serialize(data, arg, std::strlen(arg)); }

// string
  template<>
  inline void serialize<std::string>(vqueue& data, std::string& arg)
    { serialize(data, arg.data(), arg.size()); }

  template<>
  inline bool deserialize<std::string>(vqueue& data, std::string& arg)
  {
    arg.resize(data.front(), 0);
    return deserialize(data, arg.data(), arg.size());
  }

// wide string
  template<>
  inline void serialize<std::wstring>(vqueue& data, std::wstring& arg)
    { serialize(data, arg.data(), arg.size()); }

  template<>
  inline bool deserialize<std::wstring>(vqueue& data, std::wstring& arg)
  {
    arg.resize(data.front(), 0);
    return deserialize(data, arg.data(), arg.size());
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
  static inline std::vector<uint8_t> serialize(ArgTypes&... args)
  {
    vqueue d;
    serialize(d, args...);
    return d;
  }

  template<typename... ArgTypes>
  static inline bool deserialize(std::vector<uint8_t>& data, ArgTypes&... args)
  {
    vqueue d(data);
    return deserialize(d, args...);
  }
}

#endif // SERIALIZERS_H
