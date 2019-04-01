#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <put/object.h>
#include <termios.h>

namespace tui
{
  class Keyboard : public Object
  {
  public:
    Keyboard(void) noexcept;
    ~Keyboard(void) noexcept;

    signal<uint64_t> keyPressed;
  private:
    termios m_original;
    termios m_terminal;
  };
}

#endif // KEYBOARD_H
