#include "fstable.h"

// POSIX
#include <ctype.h>

// PUT
#include <specialized/osdetect.h>
#include <cxxutils/posix_helpers.h>

fsentry_t::fsentry_t(void) noexcept
  : dump_frequency(0),
    pass(0)
{
  device      = static_cast<char*>(posix::malloc(PATH_MAX));
  path        = static_cast<char*>(posix::malloc(PATH_MAX));
  filesystems = static_cast<char*>(posix::malloc(PATH_MAX));
  options     = static_cast<char*>(posix::malloc(PATH_MAX));
}

fsentry_t::fsentry_t(const char* _device,
                     const char* _path,
                     const char* _filesystems,
                     const char* _options,
                     const int _dump_frequency,
                     const int _pass) noexcept
  : fsentry_t()
{
  if(device      != NULL) { posix::strncpy(device     , _device     , PATH_MAX); }
  if(path        != NULL) { posix::strncpy(path       , _path       , PATH_MAX); }
  if(filesystems != NULL) { posix::strncpy(filesystems, _filesystems, PATH_MAX); }
  if(options     != NULL) { posix::strncpy(options    , _options    , PATH_MAX); }
  dump_frequency = _dump_frequency;
  pass = _pass;
}

fsentry_t::~fsentry_t(void) noexcept
{
  if(device      != NULL) { posix::free(device     ); }
  if(path        != NULL) { posix::free(path       ); }
  if(filesystems != NULL) { posix::free(filesystems); }
  if(options     != NULL) { posix::free(options    ); }
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
      posix::strncmp(device     , other.device      , PATH_MAX) == 0 &&
      posix::strncmp(path       , other.path        , PATH_MAX) == 0 &&
      posix::strncmp(filesystems, other.filesystems , PATH_MAX) == 0 &&
      posix::strncmp(options    , other.options     , PATH_MAX) == 0 &&
      dump_frequency == other.dump_frequency &&
      pass == other.pass;
}


#if defined(__solaris__) /* Solaris */
static const char* fs_table_path  = "/etc/vfstab";
static const char* mount_table_path = "/etc/mnttab"
#elif defined(__hpux__)  /* HP-UX   */
// No fstab
static const char* mount_table_path = "/etc/mnttab"
#elif defined(__aix__)   /* AIX     */
static const char* fs_table_path    = "/etc/filesystems";
// No mount table
#else
static const char* fs_table_path    = "/etc/fstab";
static const char* mount_table_path = "/etc/mtab";
#endif


#if defined(__linux__)    /* Linux    */ || \
    defined(__aix__)      /* AIX      */ || \
    defined(__irix__)     /* IRIX     */ || \
    defined(__hpux__)     /* HP-UX    */
# include <mntent.h>

bool parse_table(std::list<struct fsentry_t>& table, const char* filename) noexcept
{
  table.clear();
  FILE* file = ::setmntent(filename, "r");
  if(file == nullptr && posix::is_success())
    return posix::error(std::errc::no_such_file_or_directory);
  if(file == nullptr)
    return false;

  struct mntent* entry = NULL;
  while((entry = ::getmntent(file)) != NULL &&
        posix::is_success())
  {
    table.emplace_back(entry->mnt_fsname,
                       entry->mnt_dir,
                       entry->mnt_type,
                       entry->mnt_opts,
                       entry->mnt_freq,
                       entry->mnt_passno);
  }
  ::endmntent(file);
  return posix::is_success();
}

#elif defined(__solaris__)  /* Solaris  */
# include <sys/vfstab.h>
# include <ctype.h>

int decode_pass(const char* pass)
{
  if(pass[1] != '\0' || // too long OR
     std::isdigit(pass[0]) == 0) // not a digit
    return 0;
  return pass[0] - '0'; // get value
}

bool filesystem_table(std::list<struct fsentry_t>& table) noexcept
{
  table.clear();
  FILE* file = ::open(fs_table_path, "r");
  if(file == nullptr)
    return posix::error_response;

  vfstab entry;
  while(::getvfsent(file, &entry) != posix::success_response)
  {
    table.emplace_back(entry.vfs_special,
                       entry.vfs_mountp,
                       entry.vfs_fstype,
                       entry.vfs_mntopts,
                       0,
                       decode_pass(entry.vfs_fsckpass));
  }
  ::close(file);
  return posix::is_success();
}
#endif


#if defined(__linux__)    /* Linux    */ || \
    defined(__hpux__)     /* HP-UX    */ || \
    defined(__irix__)     /* IRIX     */ || \
    defined(__zos__)      /* z/OS     */

bool filesystem_table(std::list<struct fsentry_t>& table) noexcept
  { return parse_table(table, fs_table_path); }

#elif defined(BSD4_4)       /* *BSD     */ || \
      defined(__hpux__)     /* HP-UX    */ || \
      defined(__sunos__)    /* SunOS    */ || \
      defined(__aix__)      /* AIX      */ || \
      defined(__tru64__)    /* Tru64    */ || \
      defined(__ultrix__)   /* Ultrix   */
# include <fstab.h>

# if defined(__hpux__)
#  define fstab checklist
# endif

bool filesystem_table(std::list<struct fsentry_t>& table) noexcept
{
  struct fstab* entry = NULL;
  while((entry = ::getfsent()) != NULL &&
        posix::is_success())
    table.emplace_back(entry->fs_spec,
                       entry->fs_file,
                       entry->fs_vfstype,
                       entry->fs_mntops,
                       entry->fs_freq,
                       entry->fs_passno);
  return posix::is_success();
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

bool filesystem_table(std::list<struct fsentry_t>& table) noexcept
{
  struct fsentry_t entry;
  table.clear();

  posix::FILE* file = posix::fopen(fs_table_path, "r");

  if(file == NULL)
    return false;
/*
  char* field = static_cast<char*>(posix::malloc(PATH_MAX));
  if(field == NULL)
    return posix::error_response;

  posix::ssize_t count = 0;
  posix::size_t size = 0;
  char* line = NULL; // getline will malloc
  while((count = posix::getline(&line, &size, file)) != posix::error_response)
  {
    char* comment = posix::strchr(line, '#');
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
  posix::free(line);
  line = nullptr;

  posix::free(field);
  field = nullptr;
*/
  return posix::is_success();
}
#endif



#if defined(__linux__)    /* Linux    */ || \
    defined(__aix__)      /* AIX      */ || \
    defined(__irix__)     /* IRIX     */ || \
    defined(__solaris__)  /* Solaris  */ || \
    defined(__hpux__)     /* HP-UX    */

bool mount_table(std::list<struct fsentry_t>& table) noexcept
  { return parse_table(table, mount_table_path); }

#elif defined(BSD4_4)       /* *BSD     */ || \
      defined(__darwin__)   /* Darwin   */ || \
      defined(__tru64__)    /* Tru64    */

# include <sys/types.h>

# if defined(__NetBSD__) && KERNEL_VERSION_CODE < KERNEL_VERSION(3,0,0)
#  include <sys/statvfs.h>
#  define statfs statvfs;
#  define getfsstat(a,b,c) getvfsstat(a,b,c)
# else
#  include <sys/mount.h>
# endif

bool mount_table(std::list<struct fsentry_t>& table) noexcept
{
  int count = getfsstat(NULL, 0, 0);
  if(count < 0)
    return false;

  struct statfs* buffer = static_cast<struct statfs*>(posix::malloc(sizeof(struct statfs) * count));
  if(buffer == nullptr)
    return false;

  if(getfsstat(buffer, sizeof(struct statfs) * count, 0) != posix::success_response)
    return false;

  for(int i = 0; i < count; ++i)
    table.emplace_back(buffer[i].f_mntfromname,
                       buffer[i].f_mntonname,
#if (defined(__FreeBSD__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(3,0,0)) || \
    (defined(__NetBSD__)  && KERNEL_VERSION_CODE >= KERNEL_VERSION(1,0,0)) || \
     defined(__OpenBSD__) || \
     defined(__darwin__)
                       buffer[i].f_fstypename,
#else
                       "",
#endif
                       "",
                       0,
                       0);
  posix::free(buffer);
  return true;
}

#else
# pragma message("No mountpoint interrogation code for this platform! Please submit a patch.")
bool mount_table(std::list<struct fsentry_t>& table) noexcept
{
  errno = EOPNOTSUPP;
  return false;
}
#endif
