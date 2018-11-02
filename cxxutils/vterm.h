#ifndef VTERM_H
#define VTERM_H

// PUT
#include <cxxutils/posix_helpers.h>

typedef const char* const string_literal;

#undef CSI
#define CSI "\x1b["

constexpr bool strings_equal(string_literal a, string_literal b)
  { return *a == *b && (*a == '\0' || strings_equal(a + 1, b + 1)); }
static_assert(strings_equal(CSI, "\x1b["), "Preprocessor variable \"CSI\" must equal \"\\x1b[\"");

constexpr size_t string_length(string_literal str)
  { return (*str == '\0') ? 0 : (1 + string_length(str + 1)); }


namespace terminal
{
  inline bool write(const char c) noexcept { return posix::write(STDOUT_FILENO, &c, size_t(1)) != posix::error_response; }
  inline bool write(string_literal str) noexcept { return posix::write(STDOUT_FILENO, str, string_length(str)) != posix::error_response; }

  template<typename... Args>
  inline bool write(string_literal fmt, Args... args) noexcept { return ::dprintf(STDOUT_FILENO, fmt, args...) != posix::error_response; }

  inline void getWindowSize(uint16_t& rows, uint16_t& columns) noexcept
  {
#if defined(TIOCGWINSZ)
    struct winsize w;
    if(posix::ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == posix::success_response)
    {
      rows = w.ws_row;
      columns = w.ws_col;
    }
#elif defined(WIOCGETD)
    struct uwdata w;
    if(posix::ioctl(STDOUT_FILENO, WIOCGETD, &w) == posix::success_response)
    {
      if(w.uw_vs)
        rows = w.uw_height / w.uw_vs;
      if(w.uw_hs)
        columns = w.uw_width / w.uw_hs;
    }
#else
# pragma message("Incapable of get the terminal window size!  Using default values.")
    rows = 24;
    columns = 80;
#endif
  }

  inline bool hideCursor(void) noexcept { return write(CSI "?25l"); }
  inline bool showCursor(void) noexcept { return write(CSI "?25h"); }

  inline bool moveCursorUp   (uint16_t rows = 1) noexcept { return write(CSI "%huA", rows); }
  inline bool moveCursorDown (uint16_t rows = 1) noexcept { return write(CSI "%huB", rows); }
  inline bool moveCursorLeft (uint16_t cols = 1) noexcept { return write(CSI "%huC", cols); }
  inline bool moveCursorRight(uint16_t cols = 1) noexcept { return write(CSI "%huD", cols); }

  inline bool setCursorHorizontalPosition(uint16_t column) noexcept { return write(CSI "%huG", column); }
  inline bool setCursorPosition(uint16_t row, uint16_t column) noexcept { return write(CSI "%hu;%huH", row, column); }

  inline bool clearScreenAfter  (void) noexcept { return write(CSI "0J"); }
  inline bool clearScreenBefore (void) noexcept { return write(CSI "1J"); }
  inline bool clearScreen       (void) noexcept { return write(CSI "2J"); }

  inline bool clearLineAfter    (void) noexcept { return write(CSI "0K"); }
  inline bool clearLineBefore   (void) noexcept { return write(CSI "1K"); }
  inline bool clearLine         (void) noexcept { return write(CSI "2K"); }

  namespace text
  {
    string_literal reset          = CSI "0m" ;
    string_literal bold           = CSI "1m" ;
    string_literal bold_off       = CSI "22m";
    string_literal underline      = CSI "4m" ;
    string_literal underline_off  = CSI "24m";
    string_literal overline       = CSI "53m";
    string_literal overline_off   = CSI "55m";
    string_literal layerswap      = CSI "7m" ;
    string_literal layerswap_off  = CSI "27m";
    string_literal blink          = CSI "5m" ;
    string_literal blink_off      = CSI "25m";
  }

  namespace fg
  {
    string_literal black          = CSI "30m";
    string_literal red            = CSI "31m";
    string_literal green          = CSI "32m";
    string_literal yellow         = CSI "33m";
    string_literal blue           = CSI "34m";
    string_literal magenta        = CSI "35m";
    string_literal cyan           = CSI "36m";
    string_literal white          = CSI "37m";
    string_literal reset          = CSI "39m";
  }

  namespace bg
  {
    string_literal black          = CSI "40m";
    string_literal red            = CSI "41m";
    string_literal green          = CSI "42m";
    string_literal yellow         = CSI "43m";
    string_literal blue           = CSI "44m";
    string_literal magenta        = CSI "45m";
    string_literal cyan           = CSI "46m";
    string_literal white          = CSI "47m";
    string_literal reset          = CSI "49m";
  }

  namespace style
  {
    string_literal reset          = CSI "0m";
    string_literal darkRed        = CSI "0;40;31m";
    string_literal darkGreen      = CSI "0;40;32m";
    string_literal darkYellow     = CSI "0;40;33m";
    string_literal darkBlue       = CSI "0;40;34m";
    string_literal darkMagenta    = CSI "0;40;35m";
    string_literal darkCyan       = CSI "0;40;36m";
    string_literal darkWhite      = CSI "0;40;37m";
    string_literal brightRed      = CSI "0;40;31;1m";
    string_literal brightGreen    = CSI "0;40;32;1m";
    string_literal brightYellow   = CSI "0;40;33;1m";
    string_literal brightBlue     = CSI "0;40;34;1m";
    string_literal brightMagenta  = CSI "0;40;35;1m";
    string_literal brightCyan     = CSI "0;40;36;1m";
    string_literal brightWhite    = CSI "0;40;37;1m";
  }

  string_literal information      = CSI "0;40;34m"   "INFORMATION:"     CSI "0m ";
  string_literal warning          = CSI "0;40;33;1m" "WARNING:"         CSI "0m ";
  string_literal severe           = CSI "0;40;31;1m" "SEVERE WARNING:"  CSI "0m ";
  string_literal critical         = CSI "0;41;37;1m" "CRITICAL ERROR:"  CSI "0m ";
}

#endif // STREAMCOLORS_H
