#if defined(CATALOG_NAME)
#include "translate.h"

// POSIX
#include <nl_types.h> // for catalog functions

// PDTK
#include <cxxutils/hashing.h>

#define Q_uZ80A_s00(x) #x
#define Q_uZ80A_s01(x) Q_uZ80A_s00(x)

static_assert(sizeof(int) == sizeof(uint32_t), "string hash would truncate!");

static nl_catd catalog = ::catopen(Q_uZ80A_s01(CATALOG_NAME), 0); //get_catalog();

const char* operator "" _xlate(const char* str, const posix::size_t sz) noexcept
  { return ::catgets(catalog, 0, static_cast<int>(hash(str, sz)), str); }

#undef Q_uZ80A_s00
#undef Q_uZ80A_s00

#endif
