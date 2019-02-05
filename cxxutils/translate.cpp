#if defined(CATALOG_NAME)
#include "translate.h"

// PUT
#include <cxxutils/hashing.h>

#define Q_uZ80A_s00(x) #x
#define Q_uZ80A_s01(x) Q_uZ80A_s00(x)

static_assert(sizeof(int) == sizeof(uint32_t), "string hash would truncate!");

static nl_catd string_catalog = posix::catopen(Q_uZ80A_s01(CATALOG_NAME), 0);
static int32_t string_language = hash(getenv("LANG"));

void force_language(const char* const str) noexcept
  { string_language = hash(str); }

const char* operator "" _xlate(const char* str, const posix::size_t sz) noexcept
  { return posix::catgets(string_catalog, string_language, hash(str, sz) & INT32_MAX, str); }

#undef Q_uZ80A_s00
#undef Q_uZ80A_s00

#endif
