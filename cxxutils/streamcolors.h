#ifndef STREAMCOLORS_H
#define STREAMCOLORS_H

// STL
#include <ostream>

// Text Modifiers
#define RESET_TEXT          "\x1B[0m"
#define BOLD_ON_TEXT        "\x1B[1m"
#define BOLD_OFF_TEXT       "\x1B[22m"
#define UNDERLINE_ON_TEXT   "\x1B[4m"
#define UNDERLINE_OFF_TEXT  "\x1B[24m"
#define OVERLINE_ON_TEXT    "\x1B[53m"
#define OVERLINE_OFF_TEXT   "\x1B[55m"
#define LAYERSWAP_ON_TEXT   "\x1B[7m"
#define LAYERSWAP_OFF_TEXT  "\x1B[27m"
#define BLINK_ON_TEXT       "\x1B[5m"
#define BLINK_OFF_TEXT      "\x1B[25m"

#define BLACK_FG    "\x1B[30m"
#define RED_FG      "\x1B[31m"
#define GREEN_FG    "\x1B[32m"
#define YELLOW_FG   "\x1B[33m"
#define BLUE_FG     "\x1B[34m"
#define MAGENTA_FG  "\x1B[35m"
#define CYAN_FG     "\x1B[36m"
#define WHITE_FG    "\x1B[37m"
#define RESET_FG    "\x1B[39m"

#define BLACK_BG    "\x1B[40m"
#define RED_BG      "\x1B[41m"
#define GREEN_BG    "\x1B[42m"
#define YELLOW_BG   "\x1B[43m"
#define BLUE_BG     "\x1B[44m"
#define MAGENTA_BG  "\x1B[45m"
#define CYAN_BG     "\x1B[46m"
#define WHITE_BG    "\x1B[47m"
#define RESET_BG    "\x1B[49m"


namespace tty
{
  enum class text
  {
    reset,
    bold,
    bold_off,
    underline,
    underline_off,
    overline,
    overline_off,
    layerswap,
    layerswap_off,
    blink,
    blink_off,
  };

  enum class fg
  {
    black,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white,
    reset,
  };

  enum class bg
  {
    black,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white,
    reset,
  };

  static std::ostream& operator <<(std::ostream& os, const fg& name)
  {
    switch(name)
    {
      case fg::black  : return os.write(BLACK_FG   , sizeof(BLACK_FG  ));
      case fg::red    : return os.write(RED_FG     , sizeof(RED_FG    ));
      case fg::green  : return os.write(GREEN_FG   , sizeof(GREEN_FG  ));
      case fg::yellow : return os.write(YELLOW_FG  , sizeof(YELLOW_FG ));
      case fg::blue   : return os.write(BLUE_FG    , sizeof(BLUE_FG   ));
      case fg::magenta: return os.write(MAGENTA_FG , sizeof(MAGENTA_FG));
      case fg::cyan   : return os.write(CYAN_FG    , sizeof(CYAN_FG   ));
      case fg::white  : return os.write(WHITE_FG   , sizeof(WHITE_FG  ));
      case fg::reset  : return os.write(RESET_FG   , sizeof(RESET_FG  ));
    }
    return os;
  }

  static std::ostream& operator <<(std::ostream& os, const bg& name)
  {
    switch(name)
    {
      case bg::black  : return os.write(BLACK_BG   , sizeof(BLACK_BG  ));
      case bg::red    : return os.write(RED_BG     , sizeof(RED_BG    ));
      case bg::green  : return os.write(GREEN_BG   , sizeof(GREEN_BG  ));
      case bg::yellow : return os.write(YELLOW_BG  , sizeof(YELLOW_BG ));
      case bg::blue   : return os.write(BLUE_BG    , sizeof(BLUE_BG   ));
      case bg::magenta: return os.write(MAGENTA_BG , sizeof(MAGENTA_BG));
      case bg::cyan   : return os.write(CYAN_BG    , sizeof(CYAN_BG   ));
      case bg::white  : return os.write(WHITE_BG   , sizeof(WHITE_BG  ));
      case bg::reset  : return os.write(RESET_BG   , sizeof(RESET_BG  ));
    }
    return os;
  }

  static std::ostream& operator <<(std::ostream& os, const text& name)
  {
    switch(name)
    {
      case text::reset        : return os.write(RESET_TEXT        , sizeof(RESET_TEXT         ));
      case text::bold         : return os.write(BOLD_ON_TEXT      , sizeof(BOLD_ON_TEXT       ));
      case text::bold_off     : return os.write(BOLD_OFF_TEXT     , sizeof(BOLD_OFF_TEXT      ));
      case text::underline    : return os.write(UNDERLINE_ON_TEXT , sizeof(UNDERLINE_ON_TEXT  ));
      case text::underline_off: return os.write(UNDERLINE_OFF_TEXT, sizeof(UNDERLINE_OFF_TEXT ));
      case text::overline     : return os.write(OVERLINE_ON_TEXT  , sizeof(OVERLINE_ON_TEXT   ));
      case text::overline_off : return os.write(OVERLINE_OFF_TEXT , sizeof(OVERLINE_OFF_TEXT  ));
      case text::layerswap    : return os.write(LAYERSWAP_ON_TEXT , sizeof(LAYERSWAP_ON_TEXT  ));
      case text::layerswap_off: return os.write(LAYERSWAP_OFF_TEXT, sizeof(LAYERSWAP_OFF_TEXT ));
      case text::blink        : return os.write(BLINK_ON_TEXT     , sizeof(BLINK_ON_TEXT      ));
      case text::blink_off    : return os.write(BLINK_OFF_TEXT    , sizeof(BLINK_OFF_TEXT     ));
    }
    return os;
  }


  const char* const information = RESET_TEXT              WHITE_BG BLUE_FG   "INFORMATION:"    RESET_TEXT " ";
  const char* const warning     = RESET_TEXT BOLD_ON_TEXT BLACK_BG YELLOW_FG "WARNING:"        RESET_TEXT " ";
  const char* const severe      = RESET_TEXT BOLD_ON_TEXT BLACK_BG RED_FG    "SEVERE WARNING:" RESET_TEXT " ";
  const char* const critical    = RESET_TEXT BOLD_ON_TEXT RED_BG   WHITE_FG  "CRITICAL:"       RESET_TEXT " ";
}

#endif // STREAMCOLORS_H
