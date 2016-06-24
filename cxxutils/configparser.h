#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <string>
#include <memory>
#include <unordered_map>

struct node_t
{
  std::string value;
  std::unordered_map<std::string, std::shared_ptr<node_t>> values;

  node_t(void) noexcept;
  node_t(std::string& val) noexcept;
  std::shared_ptr<node_t> subsection(std::string& val) noexcept;
  std::shared_ptr<node_t> subsection(size_t index, std::string& val) noexcept;
};

class ConfigParser : public std::shared_ptr<node_t>
{
public:
  bool parse(const std::string& strdata) noexcept;
};

#endif // CONFIGPARSER_H
