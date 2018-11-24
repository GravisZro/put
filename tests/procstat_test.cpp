// POSIX++
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>
#include <cassert>

// Realtime POSIX
#include <spawn.h>

// POSIX
#include <unistd.h>
#include <inttypes.h> // for fscanf macros

// STL
#include <algorithm>

// PUT
#include <cxxutils/posix_helpers.h>
#include <cxxutils/vterm.h>
#include <specialized/procstat.h>

#define BUF_SIZE 0x8000

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
template<typename T, int base>
T decode(const char* str)
{ return decode<T,base>(str, str + std::strlen(str)); }

void copy_field(char* output, const char* start, const char* end)
{
  std::memset(output, 0, BUF_SIZE);
  std::memcpy(output, start, size_t(end - start));
}

void proc_test_split_arguments(std::vector<std::string>& argvector, const char* argstr)
{
  std::string str;
  bool in_quote = false;
  str.reserve(NAME_MAX);
  for(const char* pos = argstr; *pos; ++pos)
  {
    if(*pos == '"')
      in_quote ^= true;
    else if(in_quote)
    {
      if(*pos == '\\')
      {
        switch(*(++pos))
        {
          case 'a' : str.push_back('\a'); break;
          case 'b' : str.push_back('\b'); break;
          case 'f' : str.push_back('\f'); break;
          case 'n' : str.push_back('\n'); break;
          case 'r' : str.push_back('\r'); break;
          case 't' : str.push_back('\t'); break;
          case 'v' : str.push_back('\v'); break;
          case '"' : str.push_back('"' ); break;
          case '\\': str.push_back('\\'); break;
          case '\0': --pos; break;
          default: break;
        }
      }
      else
        str.push_back(*pos);
    }
    else if(std::isspace(*pos))
    {
      if(!str.empty())
      {
        argvector.emplace_back(str);
        str.clear();
      }
    }
    else if(std::isgraph(*pos))
      str.push_back(*pos);
  }
  if(!str.empty())
    argvector.emplace_back(str);
}


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
/*
  if(posix::is_success())
    assert(!system("rm psoutput sedoutput.*"));
*/
}

int main(int argc, char* argv[])
{
  std::atexit(file_cleanup);
  const char* ps_command  = "ps -A " \
                            "-o pid " \
                            "-o ppid " \
                            "-o pgid " \
                            "-o user=abcdefghijkl " \
                            "-o group=abcdefghijkl " \
                            "-o ruser=abcdefghijkl " \
                            "-o rgroup=abcdefghijkl " \
                            "-o tty=abcdefghijkl " \
                            "-o pcpu " \
                            "-o vsz " \
                            "-o nice " \
                            "-o time=abcdefghijkl " \
                            "-o etime=abcdefghijkl " \
                            "-o comm " \
                            "-o args > psoutput";

#define CAPTURE "\\(\\S*\\)"
#define SKIP    "\\s*"
  const char* sed_command1 = "sed -e 's/^" \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP \
                            "/\\1|\\2|\\3|\\4|\\5|\\6|\\7|\\8|\\9|/g'" \
                            " psoutput > sedoutput.1";

  const char* sed_command2 = "sed -e 's/^" \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP CAPTURE \
                            SKIP \
                            "/\\1\t\\2\t\\3\t\\4\t\\5\t/g'" \
                            " sedoutput.1 > sedoutput.2";

  const char* sed_command3 = "sed" \
                            " -e 's/|/\t/g'" \
                            " sedoutput.2 > sedoutput.3";

  std::printf("\n%s\n%s\n%s\n%s\n", ps_command, sed_command1, sed_command2, sed_command3);

  assert(!system(ps_command));
  assert(!system(sed_command1));
  assert(!system(sed_command2));
  assert(!system(sed_command3));

  process_state_t ps_state, procstat_state;

  char* field_buffer = NULL;
  size_t skip_count = 0;

  FILE* fptr = posix::fopen("sedoutput.3", "r");

  getfield(field_buffer, '\n', fptr); // skip header line

  while(!posix::feof(fptr)) // while not at end of file
  {
    if(getfield(field_buffer, '\t', fptr)) // pid
    {
      ps_state.process_id = decode<pid_t, 10>(field_buffer);
      if(!ps_state.process_id)
        std::printf("failed to read PID! '%s'\n", field_buffer);
    }
    if(getfield(field_buffer, '\t', fptr)) // ppid
    {
      ps_state.parent_process_id = decode<pid_t, 10>(field_buffer);
      if(!ps_state.parent_process_id && not_zero(field_buffer))
        std::printf("failed (PID: %i) to decode parent PID! '%s'\n", ps_state.process_id, field_buffer);
    }
    if(getfield(field_buffer, '\t', fptr)) // pgid
    {
      ps_state.process_group_id = decode<pid_t, 10>(field_buffer);
      if(!ps_state.process_group_id && not_zero(field_buffer))
        std::printf("failed (PID: %i) to decode process GID! '%s'\n", ps_state.process_id, field_buffer);
    }
    if(getfield(field_buffer, '\t', fptr)) // name
    {
      ps_state.effective_user_id = posix::getuserid(field_buffer);
      if(ps_state.effective_user_id == uid_t(posix::error_response))
        std::printf("failed (PID: %i) to find effective user id for: '%s'\n", ps_state.process_id, field_buffer);
    }
    if(getfield(field_buffer, '\t', fptr)) // group
    {
      ps_state.effective_group_id = posix::getgroupid(field_buffer);
      if(ps_state.effective_group_id == uid_t(posix::error_response))
        std::printf("failed (PID: %i) to find effective group id for: '%s'\n", ps_state.process_id, field_buffer);
    }
    if(getfield(field_buffer, '\t', fptr)) // rname
    {
      ps_state.real_user_id = posix::getuserid(field_buffer);
      if(ps_state.real_user_id == uid_t(posix::error_response))
        std::printf("failed (PID: %i) to find real user id for: '%s'\n", ps_state.process_id, field_buffer);
    }
    if(getfield(field_buffer, '\t', fptr)) // rgroup
    {
      ps_state.real_group_id = posix::getgroupid(field_buffer);
      if(ps_state.real_group_id == uid_t(posix::error_response))
        std::printf("failed (PID: %i) to find real group id for: '%s'\n", ps_state.process_id, field_buffer);
    }
    if(getfield(field_buffer, '\t', fptr)) // tty
    {
      // tty_id
    }
    if(getfield(field_buffer, '\t', fptr)) // pcpu
    {
      // percent_cpu
    }
    if(getfield(field_buffer, '\t', fptr)) // vsz
    {
      ps_state.memory_size.rss = decode<segsz_t, 10>(field_buffer);
      if(!ps_state.memory_size.rss && not_zero(field_buffer))
        std::printf("failed (PID: %i) to decode memory RSS! '%s'\n", ps_state.process_id, field_buffer);
    }
    if(getfield(field_buffer, '\t', fptr)) // nice
    {
      ps_state.nice_value = decode<int8_t, 10>(field_buffer);
      if((!ps_state.nice_value && not_zero(field_buffer)) ||
         ps_state.nice_value > 20 || ps_state.nice_value < -20)
        std::printf("failed (PID: %i) to decode nice value! '%s'\n", ps_state.process_id, field_buffer);
    }
    if(getfield(field_buffer, '\t', fptr)) // time
    {
      // time
    }
    if(getfield(field_buffer, '\t', fptr)) // etime
    {
      // etime
    }
    if(getfield(field_buffer, '\t', fptr)) // comm
      ps_state.name = field_buffer;

    if(getfield(field_buffer, '\n', fptr)) // args
      proc_test_split_arguments(ps_state.arguments, field_buffer);

    if(!procstat(ps_state.process_id, procstat_state) &&
       ps_state.process_id)
    {
      flaw(skip_count > 1,
           terminal::critical,,EXIT_FAILURE,
           "too many processes missing!")
      ++skip_count;
      continue;
    }

    flaw(ps_state.process_id != procstat_state.process_id,
         terminal::critical,,EXIT_FAILURE,
         "'process_id' does not match.\nps value: %d\nprocstat value: %d",
         ps_state.process_id, procstat_state.process_id)

    flaw(ps_state.parent_process_id != procstat_state.parent_process_id,
         terminal::critical,,EXIT_FAILURE,
         "'parent_process_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
         ps_state.process_id, ps_state.parent_process_id, procstat_state.parent_process_id)

    flaw(ps_state.process_group_id != procstat_state.process_group_id,
         terminal::critical,,EXIT_FAILURE,
         "'process_group_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
         ps_state.process_id, ps_state.process_group_id, procstat_state.process_group_id)


    flaw(ps_state.effective_user_id != procstat_state.effective_user_id,
         terminal::critical,,EXIT_FAILURE,
         "'effective_user_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
         ps_state.process_id, ps_state.effective_user_id, procstat_state.effective_user_id)

    flaw(ps_state.effective_group_id != procstat_state.effective_group_id,
         terminal::critical,,EXIT_FAILURE,
         "'effective_group_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
         ps_state.process_id, ps_state.effective_group_id, procstat_state.effective_group_id)

    flaw(ps_state.real_user_id != procstat_state.real_user_id,
         terminal::critical,,EXIT_FAILURE,
         "'real_user_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
         ps_state.process_id, ps_state.real_user_id, procstat_state.real_user_id)

    flaw(ps_state.real_group_id != procstat_state.real_group_id,
         terminal::critical,,EXIT_FAILURE,
         "'real_group_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
         ps_state.process_id, ps_state.real_group_id, procstat_state.real_group_id)

    flaw(ps_state.nice_value != procstat_state.nice_value,
         terminal::critical,,EXIT_FAILURE,
         "'nice_value' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
         ps_state.process_id, ps_state.nice_value, procstat_state.nice_value)

    bool bad_args = false;
    if(ps_state.arguments != procstat_state.arguments)
    {
      auto begin = procstat_state.arguments.begin();
      auto end   = procstat_state.arguments.end();
      auto iter  = std::find(begin, end, "");
      bad_args = iter == end; // Linux kernel procs have hidden arguments!
      bad_args &= ps_state.arguments.front().at(0) == '['; // ps return lies for Linux kernel procs!
    }

    flaw(bad_args,
         terminal::critical,,EXIT_FAILURE,
         "'arguments' does not match.\nPID: %d\nps arg count: %ld\nprocstat arg count: %ld",
         ps_state.process_id, ps_state.arguments.size(), procstat_state.arguments.size());
  }
  posix::fclose(fptr);

  if(field_buffer != NULL)
    ::free(field_buffer);

  std::printf("TEST PASSED!\n");
  return EXIT_SUCCESS;
}

#if 0

int main(int argc, char* argv[])
{
  enum {
    Read = 0,
    Write = 1,
  };

  posix::fd_t stdin_pipe[2];
  posix::fd_t stdout_pipe[2];
  posix::fd_t stderr_pipe[2];
  posix_spawn_file_actions_t action;

  flaw(!posix::pipe(stdin_pipe ) ||
       !posix::pipe(stdout_pipe) ||
       !posix::pipe(stderr_pipe),
       terminal::critical,,EXIT_FAILURE,
       "Unable to create a pipe: %s", std::strerror(errno))

  posix_spawn_file_actions_init(&action);

  posix_spawn_file_actions_addclose(&action, stdin_pipe [Write]);
  posix_spawn_file_actions_addclose(&action, stdout_pipe[Read ]);
  posix_spawn_file_actions_addclose(&action, stderr_pipe[Read ]);

  posix_spawn_file_actions_adddup2 (&action, stdin_pipe [Read ], STDIN_FILENO );
  posix_spawn_file_actions_adddup2 (&action, stdout_pipe[Write], STDOUT_FILENO);
  posix_spawn_file_actions_adddup2 (&action, stderr_pipe[Write], STDERR_FILENO);

  posix_spawn_file_actions_addclose(&action, stdin_pipe [Read ]);
  posix_spawn_file_actions_addclose(&action, stdout_pipe[Write]);
  posix_spawn_file_actions_addclose(&action, stderr_pipe[Write]);

#define CAPTURE "\\(\\S\\+\\)"
#define SKIP    "\\s\\+"
/*
  // ps -A -o pid= -o ppid= -o pgid= -o user= -o group= -o ruser= -o rgroup= -o tty= -opcpu= -o vsz= -o nice= -o time= -o etime= -o comm= -o args=
  const char* args[4] ={ "sh", "-c", "ps -A -o pid= -o ppid= -o pgid= -o user= -o group= -o ruser= -o rgroup= -o tty= -opcpu= -o vsz= -o nice= -o time= -o etime= -o comm= -o args= | "
                         "sed -e 's/"
                         "^\\s*\\" CAPTURE
                         SKIP CAPTURE \
                         SKIP CAPTURE \
                         SKIP CAPTURE \
                         SKIP CAPTURE \
                         SKIP CAPTURE \
                         SKIP CAPTURE \
                         SKIP CAPTURE \
                         SKIP CAPTURE \
                         SKIP \
                         "/\\1:\\2:\\3:\\4:\\5:\\6:\\7:\\8:\\9:/'" \
                         " -e 's/" \
                         SKIP CAPTURE \
                         SKIP CAPTURE \
                         SKIP CAPTURE \
                         SKIP \
                         "/\\t\\1\\t\\2\\t\\3\\t/'",
                         NULL };
*/
  const char* args[33] ={ "ps", "-A",
                           "-o", "pid",
                           "-o", "ppid",
                           "-o", "pgid",
                           "-o", "user=------------",
                           "-o", "group=------------",
                           "-o", "ruser=------------",
                           "-o", "rgroup=------------",
                           "-o", "tty=------------",
                           "-o", "pcpu",
                           "-o", "vsz",
                           "-o", "nice",
                           "-o", "time=------------",
                           "-o", "etime=------------",
                           "-o", "comm",
                           "-o", "args",
                           NULL };

  pid_t pspid = 0;
  flaw(posix_spawnp(&pspid,
                    "/bin/ps", &action, NULL,
                    static_cast<char* const*>(const_cast<char**>(args)), NULL) != posix::success_response,
       terminal::critical,,EXIT_FAILURE,
       "posix_spawnp failed with error: %s", std::strerror(errno))


  sleep(1);
  posix::donotblock(stdout_pipe[Read]);
  //FILE* fptr = ::fdopen(stdout_pipe[Read], "r");

  char* data_buffer  = static_cast<char*>(::malloc(BUF_SIZE));
  char* field_buffer = static_cast<char*>(::malloc(BUF_SIZE));
  char* line_buffer  = static_cast<char*>(::malloc(BUF_SIZE));

  std::memset(data_buffer , 0, BUF_SIZE);
  std::memset(field_buffer, 0, BUF_SIZE);
  std::memset(line_buffer , 0, BUF_SIZE);

  process_state_t ps_state, procstat_state;
  ssize_t count = 0;
  ssize_t remaining = 0;
  ssize_t total = 0;
  ssize_t processed = 0;
  errno = 0;
  char* end = data_buffer;
  char* pos = data_buffer;
  char* next = nullptr;
  char* nextline = data_buffer;
  bool first = true;
  do
  {
    size_t movecount = size_t(nextline - data_buffer);
    if(movecount)
    {
      assert(remaining > 0);
      remaining -= movecount;
      processed += movecount;
      ::memmove(data_buffer, pos, BUF_SIZE - movecount);
      nextline = pos = data_buffer;
    }

    if (remaining < BUF_SIZE / 2)
    {
      std::printf("bytes buffered: %li - requesting %li bytes - ", remaining, BUF_SIZE - remaining);
      count = posix::read(stdout_pipe[Read], pos + remaining, size_t(BUF_SIZE - remaining));
      if(count > 0)
      {
        remaining += count;
        total += count;
      }

      std::printf("bytes returned: %li  - processed bytes: %li - total bytes: %li\n", count, processed, total);
    }
    else
    {
      count = 0;
      std::printf("bytes buffered: %li - processed bytes: %li - total bytes: %li\n", remaining, processed, total);
    }

    if(remaining > 0)
    {
      next = pos;
      end = pos + remaining;
      for(nextline = pos; *nextline != '\n' && nextline != end; ++nextline);

      assert(nextline != pos);

      copy_field(line_buffer, pos, nextline);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.process_id = decode<pid_t, 10>(pos, next);

      if(first ||
         ps_state.process_id == pspid ||
         ps_state.process_id == ::getpid())
      {
        first = false;
        pos = nextline + 1;
        continue;
      }

      if(!ps_state.process_id)
        std::printf("failed to read PID! '%s'\nline: '%s'\n", field_buffer, line_buffer);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.parent_process_id = decode<pid_t, 10>(pos, next);
      if(!ps_state.parent_process_id && pos[0] != '0' && pos+1 != next)
        std::printf("failed (PID: %i) to decode parent PID! '%s'\nline: '%s'\n", ps_state.process_id, field_buffer, line_buffer);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.process_group_id = decode<pid_t, 10>(pos, next);
      if(!ps_state.process_group_id && pos[0] != '0' && pos+1 != next)
        std::printf("failed (PID: %i) to decode process GID! '%s'\nline: '%s'\n", ps_state.process_id, field_buffer, line_buffer);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.effective_user_id = posix::getuserid(field_buffer);
      if(ps_state.effective_user_id == uid_t(posix::error_response))
        std::printf("failed (PID: %i) to find effective user id for: '%s'\nline: '%s'\n", ps_state.process_id, field_buffer, line_buffer);


      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.effective_group_id = posix::getgroupid(field_buffer);
      if(ps_state.effective_group_id == uid_t(posix::error_response))
        std::printf("failed (PID: %i) to find effective group id for: '%s'\nline: '%s'\n", ps_state.process_id, field_buffer, line_buffer);


      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.real_user_id = posix::getuserid(field_buffer);
      if(ps_state.real_user_id == uid_t(posix::error_response))
        std::printf("failed (PID: %i) to find real user id for: '%s'\nline: '%s'\n", ps_state.process_id, field_buffer, line_buffer);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.real_group_id = posix::getgroupid(field_buffer);
      if(ps_state.real_group_id == uid_t(posix::error_response))
        std::printf("failed (PID: %i) to find real group id for: '%s'\nline: '%s'\n", ps_state.process_id, field_buffer, line_buffer);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
//    tty_id,

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
//    percent_cpu,

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.memory_size.rss = decode<segsz_t, 10>(pos, next);
      if(!ps_state.memory_size.rss && pos[0] != '0' && pos+1 != next)
        std::printf("failed (PID: %i) to decode memory RSS! '%s'\nline: '%s'\n", ps_state.process_id, field_buffer, line_buffer);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.nice_value = decode<int8_t, 10>(pos, next);
      if((!ps_state.nice_value && pos[0] != '0' && pos+1 != next) ||
         ps_state.nice_value > 20 || ps_state.nice_value < -20)
        std::printf("failed (PID: %i) to decode nice value! '%s'\nline: '%s'\n", ps_state.process_id, field_buffer, line_buffer);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
//    time,

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
//    etime,

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.name = field_buffer;

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      copy_field(field_buffer, pos, nextline);
      proc_test_split_arguments(ps_state.arguments, field_buffer);


      flaw(!procstat(ps_state.process_id, procstat_state),
           terminal::critical,,EXIT_FAILURE,
           "procstat failed to get pid: %d : err: %s", ps_state.process_id, std::strerror(errno))

      flaw(ps_state.process_id != procstat_state.process_id,
           terminal::critical,,EXIT_FAILURE,
           "process_id' does not match.\nps value: %d\nprocstat value: %d",
           ps_state.process_id, procstat_state.process_id)

      flaw(ps_state.parent_process_id != procstat_state.parent_process_id,
           terminal::critical,,EXIT_FAILURE,
           "parent_process_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
           ps_state.process_id, ps_state.parent_process_id, procstat_state.parent_process_id)

      flaw(ps_state.process_group_id != procstat_state.process_group_id,
           terminal::critical,,EXIT_FAILURE,
           "process_group_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
           ps_state.process_id, ps_state.process_group_id, procstat_state.process_group_id)


      flaw(ps_state.effective_user_id != procstat_state.effective_user_id,
           terminal::critical,,EXIT_FAILURE,
           "effective_user_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
           ps_state.process_id, ps_state.effective_user_id, procstat_state.effective_user_id)

      flaw(ps_state.effective_group_id != procstat_state.effective_group_id,
           terminal::critical,,EXIT_FAILURE,
           "effective_group_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
           ps_state.process_id, ps_state.effective_group_id, procstat_state.effective_group_id)

      flaw(ps_state.real_user_id != procstat_state.real_user_id,
           terminal::critical,,EXIT_FAILURE,
           "real_user_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
           ps_state.process_id, ps_state.real_user_id, procstat_state.real_user_id)

      flaw(ps_state.real_group_id != procstat_state.real_group_id,
           terminal::critical,,EXIT_FAILURE,
           "real_group_id' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
           ps_state.process_id, ps_state.real_group_id, procstat_state.real_group_id)

      flaw(ps_state.nice_value != procstat_state.nice_value,
           terminal::critical,,EXIT_FAILURE,
           "nice_value' does not match.\nPID: %d\nps value: %d\nprocstat value: %d",
           ps_state.process_id, ps_state.nice_value, procstat_state.nice_value)

      pos = nextline + 1;
    }
  } while(remaining > 0);

  if(data_buffer != NULL)
    ::free(data_buffer);

  if(field_buffer != NULL)
    ::free(field_buffer);

  if(line_buffer != NULL)
    ::free(line_buffer);

  std::printf("TEST PASSED!\n");
  return EXIT_SUCCESS;
}
#endif
