#ifndef CONFIGMANIP_H
#define CONFIGMANIP_H

// STL
#include <string>
#include <memory>
#include <unordered_map>
#include <list>

struct node_t
{
  enum class type_e
  {
    invalid,
    value,
    string,
    array,
    section,
    multisection,
  };

  type_e type;
  std::string value;
  std::unordered_map<std::string, std::shared_ptr<node_t>> children;

  node_t(type_e t = type_e::invalid) noexcept;
  node_t(std::string& val) noexcept; // val string is erased

  std::shared_ptr<node_t> newChild(type_e t) noexcept;
  std::shared_ptr<node_t> findChild(std::string& index) const noexcept; // index string is erased
  std::shared_ptr<node_t> getChild (std::string& index) noexcept; // index string is erased
};

struct root_node_t : std::shared_ptr<node_t>
{
  root_node_t(void) noexcept;
  std::shared_ptr<node_t> findNode(std::string path) noexcept;
  std::shared_ptr<node_t> getNode (std::string path) noexcept;

  typedef std::shared_ptr<node_t> (*NodeAction)(std::shared_ptr<node_t>& node, std::string& str);
  std::shared_ptr<node_t> lookupNode(std::string path, NodeAction func) noexcept;
};

class ConfigManip : protected root_node_t
{
public:
  using root_node_t::findNode;
  using root_node_t::getNode;

  void clear(void) noexcept;
  bool importText(const std::string& data) noexcept;
  bool exportText(std::string& data) const noexcept;
  void exportKeyPairs(std::list<std::pair<std::string, std::string>>& data) const noexcept;
};

#endif // CONFIGMANIP_H
