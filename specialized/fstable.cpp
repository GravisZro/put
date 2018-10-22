#include "fstable.h"

// POSIX++
#include <cstring>
#include <cctype>

// PDTK
#include <cxxutils/posix_helpers.h>

fsentry_t::fsentry_t(void) noexcept
  : dump_frequency(0),
    pass(0)
{
  device      = static_cast<char*>(::malloc(PATH_MAX));
  path        = static_cast<char*>(::malloc(PATH_MAX));
  filesystems = static_cast<char*>(::malloc(PATH_MAX));
  options     = static_cast<char*>(::malloc(PATH_MAX));
}

fsentry_t::fsentry_t(const char* _device,
                     const char* _path,
                     const char* _filesystems,
                     const char* _options,
                     const int _dump_frequency,
                     const int _pass) noexcept
  : fsentry_t()
{
  std::strncpy(device     , _device     , PATH_MAX);
  std::strncpy(path       , _path       , PATH_MAX);
  std::strncpy(filesystems, _filesystems, PATH_MAX);
  std::strncpy(options    , _options    , PATH_MAX);
  dump_frequency = _dump_frequency;
  pass = _pass;
}

fsentry_t::~fsentry_t(void) noexcept
{
  if(device != nullptr)
    ::free(device);
  if(path != nullptr)
    ::free(path);
  if(filesystems != nullptr)
    ::free(filesystems);
  if(options != nullptr)
    ::free(options);
  device      = nullptr;
  path        = nullptr;
  filesystems = nullptr;
  options     = nullptr;
  dump_frequency = 0;
  pass = 0;
}


#if defined(__linux__)    /* Every Linux    */ || \
    defined(__OpenBSD__)  /* Every OpenBSD  */ || \
    defined(__NetBSD__)   /* Every NetBSD   */ || \
    defined(_AIX)         /* Every AIX      */ || \
    defined(__QNX__)      /* Every QNX      */ || \
    defined(__sun)        /* Every SunOS / Solaris / OpenSolaris / OpenIndiana / illumos */ || \
    defined(__hpux)       /* Every HP-UX    */

#include <mntent.h>

int parse_table(std::set<struct fsentry_t>& table, const char* filename) noexcept
{
  table.clear();
  FILE* file = std::fopen(filename, "r");
  if(file == nullptr)
    return posix::error_response;

  struct mntent* entry = nullptr;
  while((entry = ::getmntent(file)) != nullptr)
    table.emplace(entry->mnt_fsname,
                  entry->mnt_dir,
                  entry->mnt_type,
                  entry->mnt_opts,
                  entry->mnt_freq,
                  entry->mnt_passno);
  std::fclose(file);
  return posix::success_response;
}

#elif defined(__FreeBSD__)    /* Every FreeBSD  */ || \
      defined(__DragonFly__)  /* DragonFly BSD  */

#include <fstab.h>

int parse_table(std::set<struct fsentry_t>& table, const char* filename) noexcept
{
  ::setfstab(filename);

  struct fstab* entry = nullptr;
  while((entry = ::getfsent()) != nullptr)
    table.emplace(entry->fs_spec,
                  entry->fs_file,
                  entry->fs_vfstype,
                  entry->fs_mntops,
                  entry->fs_freq,
                  entry->fs_passno);
  return posix::success_response;
}

#elif defined(__unix__) || defined(__unix)      /* Generic UNIX */ || \
      (defined(__APPLE__) && defined(__MACH__)) /* Darwin       */

# if defined(__unix__) || defined(__unix)
#  pragma message("Unrecognized UNIX variant. Using low-level parser.")
# endif

static char* skip_spaces(char* data) noexcept
{
  while(*data && std::isspace(*data))
    ++data;
  return data;
}


int parse_table(std::set<struct fsentry_t>& table, const char* filename) noexcept
{
  struct fsentry_t entry;
  table.clear();

  std::FILE* file = posix::fopen(filename, "r");

  if(file == nullptr)
    return posix::error_response;
/*
  char* field = static_cast<char*>(::malloc(PATH_MAX));
  if(field == nullptr)
    return posix::error_response;

  posix::ssize_t count = 0;
  posix::size_t size = 0;
  char* line = nullptr;
  char* begin = nullptr;
  while((count = ::getline(&line, &size, file)) != posix::error_response)
  {
    char* comment = std::strchr(line, '#');
    if(comment != nullptr)
      *comment = '\0';

    char* pos = line;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::device)>(pos, entry.device);
    if(pos == nullptr)
      continue;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::path)>(pos, entry.path);
    if(pos == nullptr)
      continue;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::filesystems)>(pos, entry.filesystems);
    if(pos == nullptr)
      continue;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::options)>(pos, entry.options);
    if(pos == nullptr)
      continue;

    pos = skip_spaces(pos);

    entry.dump_frequency = *pos;
    ++pos;
    if(!std::isspace(*pos))
      continue;

    pos = skip_spaces(pos);

    entry.pass = *pos;

    if(std::isgraph(entry.pass))
      table.push_back(entry);
  }
  ::free(line); // use C free() because we're using C getline()
  line = nullptr;

  ::free(field);
  field = nullptr;
*/
  return posix::success();
}

#else
# error This platform is not supported.
#endif
