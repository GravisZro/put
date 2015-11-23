#ifndef STREAMCOLORS_H
#define STREAMCOLORS_H

#include <ostream>

// Text Modifiers
#define RESET_TEXT    "\x1B[0m"
#define BLACK_TEXT    "\x1B[30m"
#define RED_TEXT      "\x1B[31m"
#define GREEN_TEXT    "\x1B[32m"
#define YELLOW_TEXT   "\x1B[33m"
#define BLUE_TEXT     "\x1B[34m"
#define MAGENTA_TEXT  "\x1B[35m"
#define CYAN_TEXT     "\x1B[36m"
#define WHITE_TEXT    "\x1B[37m"

namespace std
{
  enum color
  {
    none,
    black,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white
  };

  ostream& operator <<(ostream& os, const color& name)
  {
    switch(name)
    {
      case none    : return os.write(RESET_TEXT   , sizeof(RESET_TEXT  ));
      case black   : return os.write(BLACK_TEXT   , sizeof(BLACK_TEXT  ));
      case red     : return os.write(RED_TEXT     , sizeof(RED_TEXT    ));
      case green   : return os.write(GREEN_TEXT   , sizeof(GREEN_TEXT  ));
      case yellow  : return os.write(YELLOW_TEXT  , sizeof(YELLOW_TEXT ));
      case blue    : return os.write(BLUE_TEXT    , sizeof(BLUE_TEXT   ));
      case magenta : return os.write(MAGENTA_TEXT , sizeof(MAGENTA_TEXT));
      case cyan    : return os.write(CYAN_TEXT    , sizeof(CYAN_TEXT   ));
      case white   : return os.write(WHITE_TEXT   , sizeof(WHITE_TEXT  ));
    }
    return os;
  }

  const char* const information = BLUE_TEXT   "INFORMATION: "    RESET_TEXT;
  const char* const warning     = YELLOW_TEXT "WARNING: "        RESET_TEXT;
  const char* const severe      = RED_TEXT    "SEVERE WARNING: " RESET_TEXT;
  const char* const critical    = RED_TEXT    "CRITICAL: "       RESET_TEXT;

}

#endif // STREAMCOLORS_H
