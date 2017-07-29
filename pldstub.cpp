// C
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// POSIX
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/resource.h>


bool canRead(int timeout) noexcept
{
  pollfd pset = { STDIN_FILENO, POLLIN, 0 };
  return ::poll(&pset, 1, timeout) > 0; // instant timeout
}

struct typeinfo_t
{
  uint16_t bytewidth;
  uint16_t count;
};
static_assert(sizeof(typeinfo_t) == 4, "bad compiler");

static typeinfo_t typeinfo;

template<typename T>
static bool readtype(T& data)
{
  return canRead(0) &&
      ::read(STDIN_FILENO, &typeinfo, sizeof(typeinfo_t)) == sizeof(typeinfo_t) &&
      sizeof(T) == typeinfo.bytewidth * typeinfo.count &&
      ::read(STDIN_FILENO, &data, sizeof(T)) == sizeof(T);
}

template<>
bool readtype(char*& data)
{
  if(!canRead(0))
    return false;

  if(data != nullptr)
  {
    ::free(data);
    //delete[] data;
    data = nullptr;
  }

  if(::read(STDIN_FILENO, &typeinfo, sizeof(typeinfo_t)) != sizeof(typeinfo_t) || typeinfo.bytewidth != sizeof(char))
    return false;
  data = reinterpret_cast<char*>(::malloc(typeinfo.count+1));
      //new char[typeinfo.count+1];
  if(data == nullptr)
    return false;
  ::memset(data, 0, typeinfo.count+1);
  return ::read(STDIN_FILENO, data, typeinfo.count) == typeinfo.count;
}

template<typename T>
static bool writetype(const T& data)
{
  struct return_t
  {
    typeinfo_t typeinfo;
    T rcode;
  } wrapped_data = { { sizeof(T), 1 }, data };

  return sizeof(return_t) == ::write(STDOUT_FILENO, &wrapped_data, sizeof(return_t));
}

namespace posix
{
  const int success_response = 0;
  const int error_response = -1;
}

static inline int get_errno(int value)
{ return value == posix::success_response ? posix::success_response : errno; }

enum class command : uint8_t
{
  invalid = 0xFF,
  invoke = 0,
  executable,
  arguments,
  environment,
  environmentvar,
  workingdir,
  priority,
  uid,
  gid,
  euid,
  egid,
  resource
};



int main(int argc, char *argv[])
{
  char* exefile = nullptr;
  char* workingdir = nullptr;
  char* arguments[0x100] = { nullptr };

  command cmd = command::invalid;

  while(canRead(1000)) // timeout after 1 second
  {
    readtype(cmd);
    switch(cmd)
    {
      case command::invoke: // invoke the executable with the previous specified commands
      {
        if(exefile == nullptr) // assume the first argument is the executable name
          exefile = arguments[0];

        ::execv(exefile, arguments);
        ::exit(errno);
      }

      case command::executable: // specify executable name explicitly
      {
        struct stat statbuf;
        readtype(exefile);
        if(::stat(exefile, &statbuf) == posix::error_response)
          writetype<int>(errno);
        else if(statbuf.st_mode ^ S_IFREG)
          writetype<int>(EACCES);
        else if(statbuf.st_mode ^ S_IEXEC)
          writetype<int>(EACCES);
        else
          writetype<int>(posix::success_response);
        break;
      }

      case command::arguments: // specify arguments
      {
        // destroy old argument list (if it exists)
        for(uint16_t i = 0; arguments[i] != nullptr && i < 0x0100; ++i)
          ::free(arguments[i]);
          //delete[] arguments[i];
        ::memset(arguments, 0, sizeof(arguments));

        // read new argument list into buffer
        for(uint16_t i = 0; readtype(arguments[i]) && i < 0x100; ++i);

        writetype<int>(arguments[0] == nullptr ? posix::error_response : posix::success_response);
        break;
      }

      case command::environment: // specify one or more environment variables
      case command::environmentvar: // specify one environment variable
      {
        int rval = posix::success_response;
        char *key, *value;
        key = nullptr;
        value = nullptr;
        while(readtype(key  ) &&
              readtype(value) &&
              rval == posix::success_response)
          rval = ::setenv(key, value, 1);

        ::free(key);
        ::free(value);
        //delete[] key;
        //delete[] value;
        writetype<int>(get_errno(rval));
        break;
      }

      case command::workingdir: // specify the working directory
      {
        ::fprintf(stderr, "workingdir\n");
        struct stat statbuf;
        readtype(workingdir);

        if(::stat(workingdir, &statbuf) == posix::error_response)
          writetype<int>(errno);
        else if(statbuf.st_mode ^ S_IFDIR)
          writetype<int>(EACCES);
        else if(statbuf.st_mode ^ S_IEXEC)
          writetype<int>(EACCES);
        else
          writetype<int>(posix::success_response);
        break;
      }

      case command::priority: // specify process priority
      {
        int priority;
        readtype(priority);
        writetype<int>(get_errno(::setpriority(PRIO_PROCESS, getpid(), priority))); // set priority
        break;
      }

      case command::uid: // specify process user id number
      {
        uid_t uid;
        readtype(uid);
        writetype<int>(get_errno(::setuid(uid)));
        break;
      }

      case command::gid: // specify process group id number
      {
        gid_t gid;
        readtype(gid);
        writetype<int>(get_errno(::setgid(gid)));
        break;
      }

      case command::euid: // specify process effective user id number
      {
        uid_t euid;
        readtype(euid);
        writetype<int>(get_errno(::seteuid(euid)));
        break;
      }

      case command::egid: // specify process effective group id number
      {
        gid_t egid;
        readtype(egid);
        writetype<int>(get_errno(::setegid(egid)));
        break;
      }

      case command::resource: // specify a limit
      {
        int limit_id;
        rlimit val;
        readtype(limit_id);
        readtype(val.rlim_cur);
        readtype(val.rlim_max);
        writetype<int>(get_errno(::setrlimit(limit_id, &val)));
        break;
      }

      default: // bad request code
        writetype<int>(EBADRQC);
        break;
    }
  }

  if(exefile != nullptr)
    ::free(exefile);
    //delete[] exefile;

  if(workingdir != nullptr)
    ::free(exefile);
    //delete[] workingdir;

  for(uint16_t i = 0; arguments[i] != nullptr && i < 0x0100; ++i)
    ::free(arguments[i]);
    //delete[] arguments[i];

  ::fprintf(stderr, "exiting\n");
  return errno;
}
