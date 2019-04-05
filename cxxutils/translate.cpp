#include "translate.h"

// PUT
#include <put/cxxutils/hashing.h>

static_assert(sizeof(int) == sizeof(uint32_t), "string hash would truncate!");

#define invalid_catalog  nl_catd(-1)

static nl_catd string_catalog = invalid_catalog;
static int32_t string_language = hash(getenv("LANG"));

bool set_catalog(const char* const str) noexcept
{
  if(string_catalog != invalid_catalog)
    posix::catclose(string_catalog);
  return (string_catalog = posix::catopen(str, 0)) != invalid_catalog;
}

void force_language(const char* const str) noexcept
  { string_language = hash(str); }

const char* operator "" _xlate(const char* str, const posix::size_t sz) noexcept
  { return posix::catgets(string_catalog, string_language, hash(str, sz) & INT32_MAX, str); }

#undef invalid_catalog
