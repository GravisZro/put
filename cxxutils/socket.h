#ifndef SOCKET_H
#define SOCKET_H

#include <object.h>
#include "socket_helpers.h"

#include <vector>
#include <string>

#include <sys/socket.h> // for socket functions
#include <sys/un.h> // for struct sockaddr_un
#include <unistd.h> // for close()

#include <cstring>
#include <cassert>
#include <thread>

#include <map>
#include <stropts.h>


using namespace posix;

class SocketBase : public Object,
                   protected socket_t
{
public:
  SocketBase(fd_t fd);
  ~SocketBase(void);

  int bytesAvailable(void);

  static void process_socket_queue(void);

  bool connect(const char *socket_path);
  bool bind   (const char *socket_path);
  bool listen(int max_connections = SOMAXCONN);

private:
  sockaddr_t m_addr;
  static lockable<fdset_t> s_socket_indexes; // use as index into s_socket_map
  static std::map<fd_t, SocketBase*> s_socket_map;
};

#endif // SOCKET_H
