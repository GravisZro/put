#ifndef TRANSLATE_H
#define TRANSLATE_H

// PUT
#include <cxxutils/posix_helpers.h>

#if defined(CATALOG_NAME)
void force_language(const char* const str) noexcept;
const char* operator "" _xlate(const char* str, const posix::size_t sz) noexcept;
#else
#pragma message("CATALOG_NAME must be defined if you wish to use _xlate")
inline void force_language(const char* const) noexcept { }
constexpr const char* operator "" _xlate(const char* str, const posix::size_t) noexcept { return str; }
#endif


#endif // TRANSLATE_H
