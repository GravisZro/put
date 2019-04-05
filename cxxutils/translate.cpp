#include "translate.h"

// PUT
#include <put/cxxutils/hashing.h>

static_assert(sizeof(int) == sizeof(uint32_t), "string hash would truncate!");

namespace catalog
{
  static const nl_catd invalid_catalog = nl_catd(-1);
  static nl_catd handle = invalid_catalog;
  static int32_t language = hash(getenv("LANG"));

  bool open(const char* const name) noexcept
    { return close() && (handle = posix::catopen(name, 0)) != invalid_catalog; }

  bool close(void) noexcept
  {
    bool rval = true;
    if(handle != invalid_catalog &&
       (rval = posix::catclose(handle)))
      handle = invalid_catalog;
    return rval;
  }

  void force_language(const char* const str) noexcept
    { language = hash(str); }
}

const char* operator "" _xlate(const char* str, const posix::size_t sz) noexcept
  { return posix::catgets(catalog::handle, catalog::language, hash(str, sz) & INT32_MAX, str); }
