#ifndef TRANSLATE_H
#define TRANSLATE_H

// PUT
#include <put/cxxutils/posix_helpers.h>

bool set_catalog(const char* const str) noexcept;
void force_language(const char* const str) noexcept;
const char* operator "" _xlate(const char* str, const posix::size_t sz) noexcept;

#endif // TRANSLATE_H
