// POSIX
#include <assert.h>

// PUT
#include <cxxutils/posix_helpers.h>
#include <cxxutils/vterm.h>
#include <cxxutils/misc_helpers.h>
#include <specialized/blockinfo.h>


template<typename T, int base>
T decode(const char* start, const char* end)
{
  static_assert(base <= 10, "not supported");
  T value = 0;
  bool neg = *start == '-';
  if(neg)
    ++start;
  for(const char* pos = start; pos != end && std::isdigit(*pos); ++pos)
  {
    value *= base;
    value += *pos - '0';
  }
  if(neg)
    value *= T(-1);
  return value;
}

template<typename T, int base> T decode(const char* str)
{ return decode<T,base>(str, str + posix::strlen(str)); }

bool getfield(char*& lineptr,
              char delimiter,
              FILE* fptr)
{
  size_t line_size = 0;
  ssize_t count = ::getdelim(&lineptr, &line_size, delimiter, fptr);
  if(count > 0)
    lineptr[count - 1] = '\0';
  return count > 0;
}

constexpr bool not_zero(const char* data)
  { return (data[0] != '0' && data[0] != '-') || data[1] != '\0'; }


void file_cleanup(void)
{
  assert(!system("rm lsblkoutput sedoutput"));
}

#include <linux/fs.h>

int main(int argc, char* argv[])
{
  posix::atexit(file_cleanup);
  const char* lsblk_command  = "lsblk -b -n -l -o NAME,LOG-SEC,SIZE > lsblkoutput";

#define CAPTURE "\\([[:graph:]]\\{1,\\}\\)"
#define SKIP    "[[:blank:]]\\{1,\\}"
  const char* sed_command = "sed"
                             " -e 's/^[[:blank:]]*" \
                             CAPTURE SKIP \
                             CAPTURE SKIP \
                             CAPTURE \
                            "/\\/dev\\/\\1\t\\2\t\\3/'" \
                            " < lsblkoutput > sedoutput";

  assert(!system(lsblk_command));
  assert(!system(sed_command));

  char* field_buffer[3] = { NULL };
  pid_t skip_count = 0;
  pid_t processed_count = 0;

  FILE* fptr = posix::fopen("sedoutput", "r");

  getfield(field_buffer[0], '\n', fptr); // skip header line

  while(!posix::feof(fptr)) // while not at end of file
  {
    blockinfo_t info;
    if(getfield(field_buffer[0], '\t', fptr) &&
       getfield(field_buffer[1], '\t', fptr) &&
       getfield(field_buffer[2], '\n', fptr) &&
       block_info(field_buffer[0], info))
    {
      ++processed_count;

      uint32_t block_size = decode<uint32_t, 10>(field_buffer[1]);

      flaw(block_size != info.block_size,
           terminal::critical,,EXIT_FAILURE,
           "block size mismatch: lsblk: %u vs code: %u\n",
           block_size, info.block_size)


      uint64_t block_count = decode<uint64_t, 10>(field_buffer[2]) / block_size;

      flaw(block_count != info.block_count,
           terminal::critical,,EXIT_FAILURE,
           "block count mismatch: lsblk: %lu vs code: %lu\n",
           block_count, info.block_count)
      posix::printf("%s - %lu\n", field_buffer[0], block_count);
    }
    else
      ++skip_count;
  }
  posix::fclose(fptr);

  for(size_t i = 0; i < arraylength(field_buffer) - 1; ++i)
    if(field_buffer[i] != NULL)
      posix::free(field_buffer[i]);

  posix::printf("matched %i devices - skipped %i devices\n", processed_count, skip_count);
  posix::printf("TEST PASSED!\n");

  return 0;
}
