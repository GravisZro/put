#ifndef TRANSLATE_H
#define TRANSLATE_H

// PDTK
#include <cxxutils/posix_helpers.h>

#if defined(CATALOG_NAME)
const char* operator "" _xlate(const char* str, const posix::size_t sz) noexcept;
#else
#pragma message("CATALOG_NAME must be defined if you wish to use _xlate")
constexpr const char* operator "" _xlate(const char* str, const posix::size_t) noexcept { return str; }
#endif


#endif // TRANSLATE_H
