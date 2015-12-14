#ifndef SERIALIZERS_H
#define SERIALIZERS_H

// STL
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

// PDTK
#include "vqueue.h"

namespace rpc
{
// ===== interface declarations =====
  template<typename T, typename... ArgTypes>
  static void serialize(vqueue& data, T& arg, ArgTypes&... others);

  template<typename T, typename... ArgTypes>
  static bool deserialize(vqueue& data, T& arg, ArgTypes&... others);
// ==================================


// ===== serializer definitions =====

// sized array
  template<typename T>
  static inline void serialize(vqueue& data, const T* arg, uint16_t length)
  {
    data.push<uint16_t>(length);
    data.push<uint8_t>(sizeof(T));
    for(size_t i = 0; i < length; ++i)
      data.push(arg[i]);
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, T* arg, uint16_t length)
  {
    if(data.pop<uint16_t>() != length)
      return false;

    if(data.pop<uint8_t>() != sizeof(T))
      return false;

    for(size_t i = 0; i < length; ++i)
      arg[i] = data.pop<T>();
    return true;
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, const T* arg, uint16_t length)
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
    arg.resize(data.front<uint16_t>());
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
    arg.resize(data.front<uint16_t>());
    return deserialize(data, arg.data(), arg.size());
  }

// wide string
  template<>
  inline void serialize<std::wstring>(vqueue& data, std::wstring& arg)
    { serialize(data, arg.data(), arg.size()); }

  template<>
  inline bool deserialize<std::wstring>(vqueue& data, std::wstring& arg)
  {
    arg.resize(data.front<uint16_t>());
    return deserialize(data, arg.data(), arg.size());
  }

// multi-arg interface
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
}

#endif // SERIALIZERS_H
