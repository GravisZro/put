#include "fstable.h"

// POSIX++
#include <cstring>
#include <cctype>
#include <climits>

// PUT
#include <specialized/osdetect.h>
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
  if(device      != NULL) { std::strncpy(device     , _device     , PATH_MAX); }
  if(path        != NULL) { std::strncpy(path       , _path       , PATH_MAX); }
  if(filesystems != NULL) { std::strncpy(filesystems, _filesystems, PATH_MAX); }
  if(options     != NULL) { std::strncpy(options    , _options    , PATH_MAX); }
  dump_frequency = _dump_frequency;
  pass = _pass;
}

fsentry_t::~fsentry_t(void) noexcept
{
  if(device      != NULL) { ::free(device     ); }
  if(path        != NULL) { ::free(path       ); }
  if(filesystems != NULL) { ::free(filesystems); }
  if(options     != NULL) { ::free(options    ); }
  device      = nullptr;
  path        = nullptr;
  filesystems = nullptr;
  options     = nullptr;
  dump_frequency = 0;
  pass = 0;
}

bool fsentry_t::operator == (const fsentry_t& other) const
{
  return
      std::strncmp(device     , other.device      , PATH_MAX) == 0 &&
      std::strncmp(path       , other.path        , PATH_MAX) == 0 &&
      std::strncmp(filesystems, other.filesystems , PATH_MAX) == 0 &&
      std::strncmp(options    , other.options     , PATH_MAX) == 0 &&
      dump_frequency == other.dump_frequency &&
      pass == other.pass;
}


#if defined(__linux__)    /* Linux    */ || \
    defined(__solaris__)  /* Solaris  */ || \
    defined(__hpux__)     /* HP-UX    */ || \
    defined(__irix__)     /* IRIX     */ || \
    defined(__zos__)      /* z/OS     */

#include <mntent.h>

bool parse_table(std::list<struct fsentry_t>& table, const std::string& filename) noexcept
{
  table.clear();
  FILE* file = ::setmntent(filename.c_str(), "r");
  if(file == nullptr)
    return posix::error_response;

  struct mntent* entry = NULL;
  while((entry = ::getmntent(file)) != NULL &&
        errno == posix::success_response)
  {
    table.emplace_back(entry->mnt_fsname,
                       entry->mnt_dir,
                       entry->mnt_type,
                       entry->mnt_opts,
                       entry->mnt_freq,
                       entry->mnt_passno);
  }
  ::endmntent(file);
  return errno == posix::success_response;
}

#elif defined(BSD4_4)       /* *BSD     */ || \
      defined(__hpux__)     /* HP-UX    */ || \
      defined(__sunos__)    /* SunOS    */ || \
      defined(__aix__)      /* AIX      */ || \
      defined(__tru64__)    /* Tru64    */ || \
      defined(__ultrix__)   /* Ultrix   */

#include <fstab.h>

bool parse_table(std::list<struct fsentry_t>& table, const std::string& filename) noexcept
{
  ::setfstab(filename.c_str());

  struct fstab* entry = NULL;
  while((entry = ::getfsent()) != NULL &&
        errno == posix::success_response)
    table.emplace_back(entry->fs_spec,
                       entry->fs_file,
                       entry->fs_vfstype,
                       entry->fs_mntops,
                       entry->fs_freq,
                       entry->fs_passno);
  return errno == posix::success_response;
}

#elif defined(__unix__)   /* Generic UNIX */

# if !defined(__darwin__)
#  pragma message("Unrecognized UNIX variant. Using low-level parser.")
# endif

static char* skip_spaces(char* data) noexcept
{
  while(*data && std::isspace(*data))
    ++data;
  return data;
}


bool parse_table(std::list<struct fsentry_t>& table, const std::string& filename) noexcept
{
  struct fsentry_t entry;
  table.clear();

  std::FILE* file = posix::fopen(filename.c_str(), "r");

  if(file == NULL)
    return false;
/*
  char* field = static_cast<char*>(::malloc(PATH_MAX));
  if(field == NULL)
    return posix::error_response;

  posix::ssize_t count = 0;
  posix::size_t size = 0;
  char* line = NULL;
  while((count = ::getline(&line, &size, file)) != posix::error_response)
  {
    char* comment = std::strchr(line, '#');
    if(comment != NULL)
      *comment = '\0';

    char* pos = line;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::device)>(pos, entry.device);
    if(pos == NULL)
      continue;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::path)>(pos, entry.path);
    if(pos == NULL)
      continue;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::filesystems)>(pos, entry.filesystems);
    if(pos == NULL)
      continue;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::options)>(pos, entry.options);
    if(pos == NULL)
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
  return errno == posix::success_response;
}

#else
# error This platform is not supported.
#endif
