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
  static bool serialize(vqueue& data, T& arg, ArgTypes&... others);

  template<typename T, typename... ArgTypes>
  static bool deserialize(vqueue& data, T& arg, ArgTypes&... others);
// ==================================


// ===== serializer definitions =====

// sized array
  template<typename T>
  static inline bool serialize(vqueue& data, const T* arg, uint16_t length)
  {
    if(!data.push<uint16_t>(length))
      return false;

    if(!data.push<uint8_t>(sizeof(T)))
      return false;

    for(size_t i = 0; i < length; ++i)
      if(!data.push(arg[i]))
        return false;

    return true;
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, T* arg, uint16_t length)
  {
    if(data.front<uint16_t>() != length)    // length mismatch
      return false;

    if(!data.pop<uint16_t>())               // buffer underflow
      return false;

    if(data.front<uint8_t>() != sizeof(T))  // size mismatch
      return false;

    if(!data.pop<uint8_t>())                // buffer underflow
      return false;

    for(size_t i = 0; i < length; ++i)
    {
      arg[i] = data.front<T>();
      if(!data.pop<T>())                    // buffer underflow
        return false;
    }
    return true;
  }

// simple types
  template<typename T>
  static inline bool serialize(vqueue& data, T& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");
    return serialize<T>(data, &arg, 1);
  }

  template<typename T>
  static inline bool deserialize(vqueue& data, T& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");
    return deserialize<T>(data, &arg, 1);
  }

// vector of simple types
  template<typename T>
  static inline bool serialize(vqueue& data, const std::vector<T>& arg)
  {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "bad type");
    return serialize(data, arg.data(), arg.size());
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
  inline bool serialize<const char*>(vqueue& data, const char*& arg)
    { return serialize(data, arg, std::strlen(arg)); }

// string
  template<>
  inline bool serialize<std::string>(vqueue& data, std::string& arg)
    { return serialize(data, arg.data(), arg.size()); }

  template<>
  inline bool deserialize<std::string>(vqueue& data, std::string& arg)
  {
    arg.resize(data.front<uint16_t>());
    return deserialize(data, const_cast<char*>(arg.data()), arg.size()); // bad form! not guaranteed to work.
  }

// wide string
  template<>
  inline bool serialize<std::wstring>(vqueue& data, std::wstring& arg)
    { return serialize(data, arg.data(), arg.size()); }

  template<>
  inline bool deserialize<std::wstring>(vqueue& data, std::wstring& arg)
  {
    arg.resize(data.front<uint16_t>());
    return deserialize(data, const_cast<wchar_t*>(arg.data()), arg.size()); // bad form! not guaranteed to work.
  }

// multi-arg interface
  template<typename T, typename... ArgTypes>
  static inline bool serialize(vqueue& data, T& arg, ArgTypes&... others)
  {
    return serialize<T>(data, arg) &&
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
