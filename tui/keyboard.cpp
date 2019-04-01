#include "keyboard.h"

#include <put/specialized/eventbackend.h>

#include <assert.h>

#define ESCAPE_BYTE 0x1B
#define EXTEND_BYTE 0x5B

// probably only works well for Linux
uint64_t read_key_sequence(posix::fd_t fd) noexcept
{
  uint64_t key = 0;
  uint8_t* keyptr = reinterpret_cast<uint8_t*>(&key);

  posix::read(fd, keyptr, 1);
  if(*keyptr == ESCAPE_BYTE) // if this is an escape sequence
  {
    posix::read(fd, ++keyptr, 1);
    if(*keyptr == EXTEND_BYTE) // this is a multibyte escape sequence
    {
      errno = posix::success_response;
      uint8_t* end = reinterpret_cast<uint8_t*>(&key) + sizeof(key);
      do {
        posix::read(fd, ++keyptr, 1);
      } while(posix::is_success() &&
              keyptr != end && // safety first!
              (*keyptr == EXTEND_BYTE ||
               (*keyptr >= '0' && *keyptr <= '9')));
    }
  }
  return key;
}

#undef ESCAPE_BYTE
#undef EXTEND_BYTE

namespace tui
{
  Keyboard::Keyboard(void) noexcept
  {
    assert(posix::donotblock(STDIN_FILENO));
    assert(tcgetattr(STDIN_FILENO, &m_original) == posix::success_response); // get current terminal settings
    posix::memcpy(&m_terminal, &m_original, sizeof(termios));
    m_terminal.c_lflag &= tcflag_t(~ICANON); // disable the canonical mode (waits for Enter key)
    m_terminal.c_lflag &= tcflag_t(~ECHO); // disable ECHO
    assert(tcsetattr(STDIN_FILENO, TCSANOW, &m_terminal) == posix::success_response); // apply settings    

    EventBackend::add(STDIN_FILENO, EventBackend::SimplePollReadFlags,
                      [this](posix::fd_t lambda_fd, native_flags_t) noexcept
                        { Object::enqueue_copy<uint64_t>(keyPressed, read_key_sequence(lambda_fd)); }
                      );
  }

  Keyboard::~Keyboard(void) noexcept
  {
    EventBackend::remove(STDIN_FILENO, EventBackend::SimplePollReadFlags); // disconnect FD with flags from signal
    assert(tcsetattr(STDIN_FILENO, TCSANOW, &m_original) == posix::success_response); // restore original terminal settings
  }
}
