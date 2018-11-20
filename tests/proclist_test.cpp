// POSIX++
#include <cstdlib>
#include <cstdio>

// Realtime POSIX
#include <spawn.h>

// POSIX
#include <unistd.h>

// STL
#include <vector>
#include <algorithm>

// PUT
#include <cxxutils/posix_helpers.h>
#include <cxxutils/vterm.h>
#include <specialized/proclist.h>

int main(int argc, char* argv[])
{
  std::vector<pid_t> everypid;

  flaw(!proclist(everypid),
       terminal::critical,,EXIT_FAILURE,
       "proclist() failed: %s", std::strerror(errno))

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

  const char* args[5] = { "ps", "-A", "-o", "pid=", NULL };

  pid_t pspid = 0;
  flaw(posix_spawnp(&pspid,
                    "/bin/ps", &action, NULL,
                    static_cast<char* const*>(const_cast<char**>(args)), NULL) != posix::success_response,
       terminal::critical,,EXIT_FAILURE,
       "posix_spawnp failed with error: %s", std::strerror(errno))

  ::sleep(1);

  char pid_str[7] = {'\0'};
  pid_t pid = 0;

  posix::donotblock(stdout_pipe[Read]);
  size_t match_count = 0;
  while(posix::read(stdout_pipe[Read], pid_str, 6) > 0)
  {
    pid = std::atoi(pid_str);
    if(pid != pspid) // ignore ps command PID
    {
      flaw(std::find(everypid.begin(), everypid.end(), pid) == everypid.end(),
           terminal::critical,,EXIT_FAILURE,
           "unable to find PID: %d", pid);
      ++match_count;
    }
  }
  flaw(match_count != everypid.size(),
       terminal::critical,,EXIT_FAILURE,
       "PID count didn't match:\nprocstat returned: %d\npids matched: %d", everypid.size(), match_count);

  std::printf("Found %li PIDs\nTEST PASSED!\n", everypid.size());
  return EXIT_SUCCESS;
}
