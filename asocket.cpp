#include "asocket.h"

// C++
#include <cassert>
#include <cstring>
#include <cstdint>

// STL
#include <iostream>

// PDTK
#include <cxxutils/posix_helpers.h>
#include <cxxutils/error_helpers.h>
#include <cxxutils/colors.h>

#ifndef CMSG_LEN
#define CMSG_ALIGN(len) (((len) + sizeof(posix::size_t) - 1) & (posix::size_t) ~ (sizeof(posix::size_t) - 1))
#define CMSG_SPACE(len) (CMSG_ALIGN (len) + CMSG_ALIGN (sizeof (struct cmsghdr)))
#define CMSG_LEN(len)   (CMSG_ALIGN (sizeof (struct cmsghdr)) + (len))
#endif

static_assert(sizeof(uint8_t) == sizeof(char), "size mismatch!");

// NOTE: this is not an enumeration so that each value can be referenced
namespace LocalCommand
{
  static const uint64_t write  = 1;
  static const uint64_t update = 2;
  static const uint64_t exit   = 3;
}

namespace posix
{
  bool peercred(fd_t sockfd, proccred_t& cred) noexcept;
//    { return ::peercred(sockfd, cred) == posix::success_response; }
}

AsyncSocket::AsyncSocket(EDomain domain, EType type, EProtocol protocol, int flags) noexcept
  : AsyncSocket(posix::socket(domain, type, protocol, flags)) { }

AsyncSocket::AsyncSocket(posix::fd_t socket) noexcept
  : m_socket(socket),
    m_bound(false)
{
  posix::fd_t cmdio[2];
  assert(::pipe(cmdio) != posix::error_response);
  m_read_command  = cmdio[0];
  m_write_command = cmdio[1];

  EventBackend::watch(m_read_command, EventFlags::Readable);
  EventBackend::watch(m_socket, EventFlags::Readable);
}

AsyncSocket::~AsyncSocket(void) noexcept
{
  posix::write(m_write_command, &LocalCommand::exit, sizeof(uint64_t));
  m_iothread.join(); // wait for thread to exit

  if(m_socket != posix::invalid_descriptor)
    posix::close(m_socket);
  if(m_selfaddr != EDomain::unspec)
    ::unlink(m_selfaddr.sun_path);
}

bool AsyncSocket::bind(const char *socket_path, int socket_backlog) noexcept
{
  if(!has_socket())
    return false;

  assert(std::strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  m_selfaddr = socket_path;
  m_selfaddr = EDomain::local;
  return m_bound = posix::bind(m_socket, m_selfaddr, m_selfaddr.size()) &&
                   posix::listen(m_socket, socket_backlog) &&
                   async_spawn();
}

bool AsyncSocket::connect(const char *socket_path) noexcept
{
  if(!has_socket())
    return false;

  assert(std::strlen(socket_path) < sizeof(sockaddr_un::sun_path));
  posix::sockaddr_t peeraddr;
  proccred_t peercred;

  peeraddr = socket_path;
  peeraddr = EDomain::local;
  m_selfaddr = EDomain::unspec;

  return posix::connect(m_socket, peeraddr, peeraddr.size()) &&
         posix::peercred(m_socket, peercred) &&
         async_spawn() &&
         Object::enqueue(connectedToPeer, m_socket, peeraddr, peercred);
}

bool AsyncSocket::async_spawn(void) noexcept
{
  if(m_iothread.joinable()) // if thread already active
    return posix::write(m_write_command, &LocalCommand::update, sizeof(uint64_t)) != posix::error_response; // notify thread of an update

  m_iothread = std::thread(&AsyncSocket::async_io , this); // create a new thread
  return m_iothread.joinable(); // if thread creation succeeded
}

void AsyncSocket::async_io(void) noexcept // runs as it's own thread
{
  msghdr msg = {};
  iovec iov = {};
  char aux_buffer[CMSG_SPACE(sizeof(int))] = { 0 };
  uint64_t command_buffer = 0;
  posix::ssize_t byte_count = 0;

  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = aux_buffer;

  for(;;)
  {
    if(EventBackend::getevents())
    {
      for(const auto& pos : EventBackend::results)
      {
        if(pos.first == m_read_command && (pos.second.flags.isSet(EventFlags::Readable)))
        {
          command_buffer = 0;
          byte_count = posix::read(pos.first, &command_buffer, sizeof(uint64_t));
          if(byte_count == posix::error_response)
            continue;

          switch(command_buffer)
          {
            case LocalCommand::update: continue; // EventBackend::m_queue size changed
            case LocalCommand::exit:
              std::cout << "thread exiting!" << std::endl << std::flush;
              return; // thread exit
            case LocalCommand::write: // m_messages isn't empty
            {
              for(auto& msg_pos : m_messages)
              {
                msg.msg_iov = &iov;
                msg.msg_iovlen = 1;
                msg.msg_control = aux_buffer;

                iov.iov_base = msg_pos.buffer.begin();
                iov.iov_len = msg_pos.buffer.size();

                msg.msg_controllen = 0;

                if(msg_pos.fd_buffer != posix::invalid_descriptor) // if a file descriptor needs to be sent
                {
                  msg.msg_controllen = CMSG_SPACE(sizeof(int));
                  cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
                  cmsg->cmsg_level = SOL_SOCKET;
                  cmsg->cmsg_type = SCM_RIGHTS;
                  cmsg->cmsg_len = CMSG_LEN(sizeof(int));
                  *reinterpret_cast<int*>(CMSG_DATA(cmsg)) = msg_pos.fd_buffer;
                }

                byte_count = posix::sendmsg(msg_pos.socket, &msg, 0);

                if(byte_count == posix::error_response) // error
                  std::cout << std::flush << std::endl << posix::fg::red << "sendmsg error: " << std::strerror(errno) << posix::fg::reset << std::endl << std::flush;
                else
                  Object::enqueue(writeFinished, msg_pos.socket, byte_count);
              }
              m_messages.clear();
            }
          }
        }
        else if(m_bound && // if in server mode
                pos.first == m_socket && // and this is a primary socket
                pos.second.flags.isSet(EventFlags::Readable)) // and it's a read event
        {
          proccred_t peercred;
          posix::sockaddr_t peeraddr;
          socklen_t addrlen = 0;
          posix::fd_t fd = posix::accept(m_socket, peeraddr, &addrlen); // accept a new socket connection
          if(fd == posix::error_response)
            std::cout << "accept error: " << std::strerror(errno) << std::endl << std::flush;
          else
          {
            if(!EventBackend::watch(fd, EventFlags::Readable)) // monitor new socket connection
              std::cout << "watch failure: " << std::strerror(errno) << std::endl << std::flush;
            if(!posix::peercred(fd, peercred)) // get creditials of connected peer process
              std::cout << "peercred failure: " << std::strerror(errno) << std::endl << std::flush;
            Object::enqueue(connectedToPeer, fd, peeraddr, peercred);
          }
        }
        else
        {
          if(pos.second.flags.isSet(EventFlags::Disconnected | EventFlags::Error)) // connection severed or connection error
          {
            EventBackend::remove(pos.first); // stop watching for events
            posix::close(pos.first); // close connection if open
          }
          else if(pos.second.flags.isSet(EventFlags::Readable)) // read
          {
            m_incomming.socket = m_bound ? m_socket : m_read_command; // select proper output socket
            m_incomming.buffer.allocate(); // allocate 64KB buffer

            iov.iov_base = m_incomming.buffer.data();
            iov.iov_len = m_incomming.buffer.capacity();
            msg.msg_controllen = sizeof(aux_buffer);

            byte_count = posix::recvmsg(pos.first, &msg, 0);

            if(byte_count == posix::error_response)
              std::cout << posix::fg::red << "recvmsg error: " << std::strerror(errno) << posix::fg::reset << std::endl << std::flush;
            else if(!byte_count)
            {
              EventBackend::remove(pos.first); // stop watching for events
              posix::close(pos.first); // connection severed!
            }
            else if(m_incomming.buffer.expand(byte_count))
            {
              m_incomming.fd_buffer = posix::invalid_descriptor;
              if(msg.msg_controllen == CMSG_SPACE(sizeof(int)))
              {
                cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
                if(cmsg->cmsg_level == SOL_SOCKET &&
                   cmsg->cmsg_type == SCM_RIGHTS &&
                   cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
                 m_incomming.fd_buffer = *reinterpret_cast<int*>(CMSG_DATA(cmsg));
              }
              else if(msg.msg_flags)
                std::cout << posix::fg::red << "error, message flags: " << std::hex << msg.msg_flags << std::dec << posix::fg::reset << std::endl << std::flush;
              Object::enqueue(readFinished, m_incomming.socket, m_incomming);
            }
          }
          else if(pos.second.flags.isSet(EventFlags::Any))
          {
            std::cout << "unknown event at: " << pos.first << " value: " << std::hex << uint32_t(pos.second.flags.operator EventFlags()) << std::dec << std::endl << std::flush;
          }
        }
      }
    }
  }
}

bool AsyncSocket::write(message_t msg) noexcept
{
  if(!is_connected(msg.socket))
    return false;
  m_messages.push_back(msg);
  posix::write(m_write_command, &LocalCommand::write, sizeof(uint64_t));
  return true;
}
