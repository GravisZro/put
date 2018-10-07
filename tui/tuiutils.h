#ifndef TUIUTILS_H
#define TUIUTILS_H

#include <cxxutils/vterm.h>

namespace tui
{
  using terminal::hideCursor;
  using terminal::showCursor;

  using terminal::moveCursorUp;
  using terminal::moveCursorDown;
  using terminal::moveCursorLeft;
  using terminal::moveCursorRight;

  using terminal::setCursorHorizontalPosition;
  using terminal::setCursorPosition;

  inline bool saveCursorPosition(void) noexcept { return terminal::write(CSI "s"); }
  inline bool loadCursorPosition(void) noexcept { return terminal::write(CSI "u"); }
  bool getCursorPosition(uint16_t& row, uint16_t& column) noexcept;


  using terminal::clearScreenAfter;
  using terminal::clearScreenBefore;
  using terminal::clearScreen;

  using terminal::clearLineAfter;
  using terminal::clearLineBefore;
  using terminal::clearLine;

  inline bool clearScollBuffer(void) noexcept { return terminal::write(CSI "3J"); }

  inline bool scrollUp(uint16_t lines = 1) noexcept { return terminal::write(CSI "?%huS", lines); }
  inline bool scrollDown(uint16_t lines = 1) noexcept { return terminal::write(CSI "?%huT", lines); }
}

#endif // TUIUTILS_H
