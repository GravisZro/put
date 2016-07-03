#include "configparser.h"

// STL
#include <cctype>

static inline std::string use_string(std::string& str) noexcept
{
  std::string copy;
  while(std::isspace(str.back())) // remove whitespace from end
    str.pop_back();
  copy = str;
  str.clear();
  return copy;
}

node_t::node_t(void) noexcept { }
node_t::node_t(std::string& val) noexcept : value(use_string(val)) { }

bool node_t::is_array(void) noexcept
{
  for(auto& val : values)
    for(auto& character : val.first)
      if(!std::isdigit(character))
        return false;
  return true;
}


std::shared_ptr<node_t> node_t::newChild(void) noexcept
{ return values.emplace(std::to_string(values.size()), std::make_shared<node_t>()).first->second; }

std::shared_ptr<node_t> node_t::findChild(std::string& index) noexcept
{
  auto val = values.find(use_string(index)); // find index if it exists
  return val == values.end() ? nullptr : val->second;
}

std::shared_ptr<node_t> node_t::getChild(std::string& index) noexcept
{ return values.emplace(use_string(index), std::make_shared<node_t>()).first->second; } // insert if index does _not_ exist


ConfigParser::ConfigParser(void) noexcept
  : std::shared_ptr<node_t>(std::make_shared<node_t>())
{
}

std::shared_ptr<node_t> ConfigParser::findNode(std::string path) noexcept
{ return lookupNode(path, [](std::shared_ptr<node_t>& node, std::string& str) noexcept { return node->findChild(str); }); }

std::shared_ptr<node_t> ConfigParser::getNode(std::string path) noexcept
{ return lookupNode(path, [](std::shared_ptr<node_t>& node, std::string& str) noexcept { return node->getChild(str); }); }

std::shared_ptr<node_t> ConfigParser::lookupNode(std::string path, NodeAction func) noexcept
{
  std::shared_ptr<node_t> node = *this;
  std::string str;
  str.reserve(256);
  for(auto& letter : path)
  {
    if(node == nullptr)
      break;
    if(letter == '/')
    {
      if(str.empty())
        node = *this;
      else
        node = func(node, str);
    }
    else
      str.push_back(letter);
  }
  if(!str.empty())
    node = func(node, str);
  return node;
}

bool ConfigParser::parse(const std::string& strdata) noexcept
{
  enum state_e
  {
    searching = 0, // new line
    section   = 1, // found section
    name      = 2, // found name
    equals    = 3, // found '=' sign
    value     = 4, // found value
    quote     = 5,
    comment   = 6,
  };

  const char* data = strdata.data();
  const char* end = data + strdata.size();

  std::shared_ptr<node_t> node = nullptr;
  std::shared_ptr<node_t> section_node = nullptr;
  std::string str;
  str.reserve(4096);

  state_e state = searching;
  state_e prev_state = state;

  for(const char* pos = data; pos < end; ++pos)
  {
    if(pos[0] == '\\' && pos[1] == '\n') // line continuation feature; applicable _ANYWHERE_
    {
      ++pos;
      continue;
    }

    switch(state)
    {
      case searching:

        switch(*pos)
        {
          default:
            if(!std::isspace(*pos))
            {
              node = section_node;
              state = name;
              --pos; // moots increment
            }
            continue;

          case '[':
            node = section_node = *this;
            state = section;
            continue;

          case ']':
          case '=':
          case '"':
          case ',':
          case '\\':
            return false;

          case ';':
          case '#':
            state = comment;
            continue;
        }

      case section:

        switch(*pos)
        {
          default:
            if(!std::isspace(*pos) || !str.empty())
              str.push_back(*pos);
          case '\n':
            continue;

          case '[':
          case '=':
          case '"':
          case ',':
          case '\\':
            return false;


          case '#':
          case ';':
            prev_state = state;
            state = comment;
            continue;

          case '/':
            if(str.empty())
              return false;
            node = section_node = section_node->getChild(str); // goto subsection
            continue;


          case ']':
            if(str.empty())
              return false;
            node = section_node = section_node->getChild(str);
            state = searching;

            if(!node->value.empty()) // value name / section name conflict
              return false;

            if(!node->values.empty()) // if section already exists
            {
              if(!node->is_array()) // if not already a multi-section
              {
                std::unordered_map<std::string, std::shared_ptr<node_t>> vals = node->values;
                node->values.clear();
                node->newChild()->values = vals;
              }
              node = node->newChild();
            }
            continue;
        }

      case name:
        switch(*pos)
        {
          default:
            if(!std::isspace(*pos))
              str.push_back(*pos);
          case '\n':
            continue;

          case '[':
          case ']':
          case '"':
          case ',':
          case '\\':
            return false;

          case '=':
            node = node->getChild(str);
            state = equals;
            continue;

          case '/':
            if(!str.empty()) // subsection name stored
              node = node->getChild(str);
            else if(node == section_node) // root forward slash
              node = *this;
            else // double foward slash
              return false;
            continue;

          case ';':
          case '#':
            prev_state = state;
            state = comment;
            continue;
        }

      case equals:
        switch(*pos)
        {
          default:
            if(!std::isspace(*pos))
            {
              state = value;
              --pos;
            }
            continue;

          case '"':
            prev_state = state;
            state = quote;
            continue;

          case ',':
            node->newChild()->value = use_string(str);
            state = value;
            continue;

          case ';':
          case '#':
            prev_state = state;
            state = comment;
            continue;

          case '=':
            return false;
        }

      case value:
        switch(*pos)
        {
          default:
            if(!std::isspace(*pos))
              str.push_back(*pos);
            continue;

          case '\n':
            if(!str.empty())
            {
              if(!node->values.empty())
                node->newChild();
              node->value = use_string(str);
            }
            state = searching;
            continue;

          case ';':
          case '#':
            prev_state = state;
            state = comment;
            continue;

          case ',':
            node->newChild()->value = use_string(str);
            continue;
        }

      case quote:

      case comment:
        switch(*pos)
        {
          case '\n':
            state = prev_state;
            --pos;
          default:
            continue;
        }
    }
  }
  return true;
}
