#include "tuiutils.h"

namespace tui
{
  bool getCursorPosition(uint16_t& row, uint16_t& column) noexcept
  {
    if(!terminal::write(CSI "6n"))
      return false;

    char buffer[sizeof(CSI "65535;65535R")] = { 0 };
    posix::ssize_t count = posix::read(STDIN_FILENO, buffer, sizeof(buffer));

    if(count == posix::error_response)
      return posix::error_response;

    if (std::sscanf(buffer, CSI "%hu;%huR", &row, &column) != 2)
      return posix::error(EINVAL);

    return true;
  }


}
