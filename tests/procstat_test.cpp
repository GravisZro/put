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
    value *= -1;
  return value;
}

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
                           "-o", "pid=",
                           "-o", "ppid=",
                           "-o", "pgid=",
                           "-o", "user=",
                           "-o", "group=",
                           "-o", "ruser=",
                           "-o", "rgroup=",
                           "-o", "tty=",
                           "-o", "pcpu=",
                           "-o", "vsz=",
                           "-o", "nice=",
                           "-o", "time=",
                           "-o", "etime=",
                           "-o", "comm=",
                           "-o", "args=",
                           NULL };

  pid_t pspid = 0;
  flaw(posix_spawnp(&pspid,
                    "/bin/ps", &action, NULL,
                    static_cast<char* const*>(const_cast<char**>(args)), NULL) != posix::success_response,
       terminal::critical,,EXIT_FAILURE,
       "posix_spawnp failed with error: %s", std::strerror(errno))

  ::sleep(1);

  posix::donotblock(stdout_pipe[Read]);
  //FILE* fptr = ::fdopen(stdout_pipe[Read], "r");

  char* line_buffer  = static_cast<char*>(::malloc(BUF_SIZE));
  char* field_buffer = static_cast<char*>(::malloc(BUF_SIZE));
  process_state_t ps_state;
  ssize_t count = 0;
  ssize_t remaining = 0;
  errno = 0;
  char* end = line_buffer;
  char* pos = line_buffer;
  char* next = nullptr;
  char* nextline = line_buffer;
  do
  {
    size_t movecount = size_t(nextline - line_buffer);
    if(movecount)
    {
      assert(remaining > 0);
      remaining -= movecount;
      ::memmove(line_buffer, pos, BUF_SIZE - movecount);
      nextline = pos = line_buffer;
    }

    count = posix::read(stdout_pipe[Read], pos + remaining, BUF_SIZE - remaining);
    if(count > 0)
      remaining += count;
    if(remaining > 0)
    {
      next = pos;
      end = pos + remaining;
      for(nextline = pos; *nextline != '\n' && nextline != end; ++nextline);

      assert(nextline != pos);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      ps_state.process_id = decode<pid_t, 10>(pos, next);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      ps_state.parent_process_id = decode<pid_t, 10>(pos, next);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      ps_state.process_group_id = decode<pid_t, 10>(pos, next);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      ps_state.effective_user_id = posix::getuserid(field_buffer);


      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.effective_group_id = posix::getgroupid(field_buffer);


      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.real_user_id = posix::getuserid(field_buffer);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      copy_field(field_buffer, pos, next);
      ps_state.real_group_id = posix::getgroupid(field_buffer);

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
      ps_state.memory_size.rss = decode<segsz_t, 10>(pos, next);

      for(pos = next; std::isspace(*pos ) && pos  != nextline; ++pos);
      for(next = pos;!std::isspace(*next) && next != nextline;++next);
      ps_state.nice_value = decode<int8_t, 10>(pos, next);

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

      pos = nextline + 1;
    }
  } while(remaining > 0);

  if(line_buffer != NULL)
    ::free(line_buffer);

  if(field_buffer != NULL)
    ::free(field_buffer);

  return EXIT_SUCCESS;
}
