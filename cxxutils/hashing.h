#ifndef HASHING_H
#define HASHING_H

// STL
#include <string>

template<typename T> constexpr unsigned int hash_array(const T* key, std::size_t size) noexcept
 { return size ? static_cast<unsigned int>(*key) + 33 * hash_array(key + 1, size - 1) : 5381; }

constexpr unsigned int operator "" _hash(const char* s, std::size_t sz) noexcept { return hash_array(s, sz); }
static inline unsigned int hash(const std::string& key) noexcept { return hash_array(key.data(), key.size()); }

#endif // HASHING_H

