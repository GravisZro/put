#ifndef STREAMCOLORS_H
#define STREAMCOLORS_H

typedef const char* const string_literal;

namespace posix
{
  namespace text
  {
    string_literal reset         = "\x1B[0m";
    string_literal bold          = "\x1B[1m";
    string_literal bold_off      = "\x1B[22m";
    string_literal underline     = "\x1B[4m" ;
    string_literal underline_off = "\x1B[24m";
    string_literal overline      = "\x1B[53m";
    string_literal overline_off  = "\x1B[55m";
    string_literal layerswap     = "\x1B[7m" ;
    string_literal layerswap_off = "\x1B[27m";
    string_literal blink         = "\x1B[5m" ;
    string_literal blink_off     = "\x1B[25m";
  }

  namespace fg
  {
    string_literal black   = "\x1B[30m";
    string_literal red     = "\x1B[31m";
    string_literal green   = "\x1B[32m";
    string_literal yellow  = "\x1B[33m";
    string_literal blue    = "\x1B[34m";
    string_literal magenta = "\x1B[35m";
    string_literal cyan    = "\x1B[36m";
    string_literal white   = "\x1B[37m";
    string_literal reset   = "\x1B[39m";
  }

  namespace bg
  {
    string_literal black   = "\x1B[40m";
    string_literal red     = "\x1B[41m";
    string_literal green   = "\x1B[42m";
    string_literal yellow  = "\x1B[43m";
    string_literal blue    = "\x1B[44m";
    string_literal magenta = "\x1B[45m";
    string_literal cyan    = "\x1B[46m";
    string_literal white   = "\x1B[47m";
    string_literal reset   = "\x1B[49m";
  }

  string_literal information = "\x1B[0;40;34mINFORMATION:\x1B[0m ";
  string_literal warning     = "\x1B[0;40;33;1mWARNING:\x1B[0m ";
  string_literal severe      = "\x1B[0;40;31;1mSEVERE WARNING:\x1B[0m ";
  string_literal critical    = "\x1B[0;41;37;1mCRITICAL ERROR:\x1B[0m ";
}

#endif // STREAMCOLORS_H
