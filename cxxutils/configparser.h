#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

// STL
#include <string>
#include <memory>
#include <unordered_map>

struct node_t
{
  std::string value;
  std::unordered_map<std::string, std::shared_ptr<node_t>> values;

  node_t(void) noexcept;
  node_t(std::string& val) noexcept; // val string is erased

  bool is_array(void) const noexcept;
  std::shared_ptr<node_t> newChild(void) noexcept;
  std::shared_ptr<node_t> findChild(std::string& index) const noexcept; // index string is erased
  std::shared_ptr<node_t> getChild (std::string& index) noexcept; // index string is erased
};

class ConfigParser : public std::shared_ptr<node_t>
{
public:
  ConfigParser(void) noexcept;
  bool parse(const std::string& strdata) noexcept;

  std::shared_ptr<node_t> findNode(std::string path) noexcept;
  std::shared_ptr<node_t> getNode (std::string path) noexcept;

private:
  typedef std::shared_ptr<node_t> (*NodeAction)(std::shared_ptr<node_t>& node, std::string& str);
  std::shared_ptr<node_t> lookupNode(std::string path, NodeAction func) noexcept;
};

#endif // CONFIGPARSER_H
