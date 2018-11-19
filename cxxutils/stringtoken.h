#ifndef STRINGTOKEN_H
#define STRINGTOKEN_H

class StringToken
{
public:
  StringToken(const char* origin = nullptr) noexcept;
  void setOrigin(const char* origin) noexcept { m_pos = origin; }
  const char* next(char token) noexcept;
  const char* next(const char* tokens) noexcept;
  const char* pos(void) const noexcept { return m_pos; }
private:
  const char* m_pos;
};

#endif // STRINGTOKEN_H
