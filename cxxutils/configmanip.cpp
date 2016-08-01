#include "configmanip.h"

// C++
#include <cctype>

// STL
#include <map>

static inline std::string use_string(std::string& str) noexcept
{
  std::string copy;
  while(std::isspace(str.back())) // remove whitespace from end
    str.pop_back();
  copy = str;
  str.clear();
  return copy;
}

node_t::node_t(void) noexcept : type(type_e::invalid) { }
node_t::node_t(std::string& val) noexcept : type(type_e::value), value(use_string(val)) { }

std::shared_ptr<node_t> node_t::newChild(type_e t) noexcept
{
  auto& node = values.emplace(std::to_string(values.size()), std::make_shared<node_t>()).first->second;
  node->type = t;
  return node;
}

std::shared_ptr<node_t> node_t::findChild(std::string& index) const noexcept
{
  auto val = values.find(use_string(index)); // find index if it exists
  return val == values.end() ? nullptr : val->second;
}

std::shared_ptr<node_t> node_t::getChild(std::string& index) noexcept
  { return values.emplace(use_string(index), std::make_shared<node_t>()).first->second; } // insert if index does _not_ exist


root_node_t::root_node_t(void) noexcept
  : std::shared_ptr<node_t>(std::make_shared<node_t>())
  { (*this)->type = node_t::type_e::section; }

std::shared_ptr<node_t> root_node_t::findNode(std::string path) noexcept
  { return lookupNode(path, [](std::shared_ptr<node_t>& node, std::string& str) noexcept { return node->findChild(str); }); }

std::shared_ptr<node_t> root_node_t::getNode(std::string path) noexcept
  { return lookupNode(path, [](std::shared_ptr<node_t>& node, std::string& str) noexcept { return node->getChild(str); }); }

std::shared_ptr<node_t> root_node_t::lookupNode(std::string path, NodeAction func) noexcept
{
  std::shared_ptr<node_t> node = *this;
  std::string str;
  str.reserve(256);
  for(auto& letter : path)
  {
    if(node == nullptr)
      return nullptr;
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

//#include <cstdlib>
constexpr
bool bailout(void)
{
//  ::abort();
  return false;
}

bool ConfigManip::read(const std::string& data) noexcept
{
  enum class state_e
  {
    searching = 0, // new line
    section,       // found section
    name,          // found name
    value,         // found value
    quote,         // found opening double quotation mark
    comment,       // found semicolon outside of quotation
  };

  const char* begin = data.data();
  const char* end = begin + data.size();

  std::shared_ptr<node_t> node = nullptr;
  std::shared_ptr<node_t> section_node = nullptr;
  std::string str;
  str.reserve(4096); // values _exceeding_ 4096 bytes will likely incur a reallocation speed penalty

  state_e state = state_e::searching;
  state_e prev_state = state;


  for(const char* pos = begin; pos < end; ++pos)
  {
    if(pos[0] == '\\' && pos[1] == '\n') // line continuation feature; applicable _ANYWHERE_
    {
      ++pos;
      continue;
    }

    switch(state)
    {
      case state_e::searching:
        switch(*pos)
        {
          default:
            if(!std::isspace(*pos))
            {
              node = section_node;
              state = state_e::name;
              --pos; // moots increment
            }
            continue;

          case '[':
            node = section_node = *this;
            state = state_e::section;
            continue;

          case ']':
          case '=':
          case '"':
          case ',':
          case '\\':
            return bailout();

          case '#':
          case ';':
            prev_state = state;
            state = state_e::comment;
            continue;
        }

      case state_e::section:
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
            return bailout();

          case ';':
            prev_state = state;
            state = state_e::comment;
            continue;

          case '/':
            if(str.empty())
              return bailout();
            node = section_node = section_node->getChild(str); // goto subsection
            continue;


          case ']':
            if(str.empty())
              return bailout();
            node = section_node = section_node->getChild(str);
            state = state_e::searching;

            switch(node->type)
            {
              case node_t::type_e::section:
              {
                std::unordered_map<std::string, std::shared_ptr<node_t>> vals = node->values;
                node->values.clear();
                node->newChild(node_t::type_e::section)->values = vals;
                node->type = node_t::type_e::multisection;
                node = node->newChild(node_t::type_e::section);
                continue;
              }

              case node_t::type_e::multisection:
                node = node->newChild(node_t::type_e::section);
                continue;

              case node_t::type_e::invalid:
                node->type = node_t::type_e::section;
                continue;

              default: // value name / section name conflict
                return bailout();
            }
        }

      case state_e::name:
        switch(*pos)
        {
          default:
            if(!std::isspace(*pos))
              str.push_back(*pos);
            continue;

          case '\n':
          case '[':
          case ']':
          case '"':
          case ',':
          case '\\':
            return bailout();

          case '=':
            node = node->getChild(str);
            state = state_e::value;
            continue;

          case '/':
            if(!str.empty()) // subsection name stored
              node = node->getChild(str);
            else if(node == section_node) // root forward slash
              node = *this;
            else // double foward slash
              return bailout();
            continue;

          case ';':
            prev_state = state;
            state = state_e::comment;
            continue;
        }

      case state_e::value:
        switch(*pos)
        {
          default:
            if(!std::isspace(*pos) ||
               (!str.empty() &&
                !std::isspace(str.back()))) // if not a space or string doesnt end with a space
              str.push_back(*pos);
            continue;

          case '=':
            return bailout();

          case '"':
            if(!str.empty()) // if quote is in the middle of a value rather than the start of it...
              return bailout();
            prev_state = state;
            state = state_e::quote;
            continue;

          case '\n':
            if(!str.empty())
            {
              if(node->type == node_t::type_e::array) // if part of a list
                node = node->newChild(node_t::type_e::value); // make new list entry
              else
                node->type = node_t::type_e::value;
              node->value = use_string(str); // store value to current node
            }
            state = state_e::searching;
            continue;

          case ';':
            prev_state = state;
            state = state_e::comment;
            continue;

          case ',':
            node->type = node_t::type_e::array;
            node->newChild(node_t::type_e::value)->value = use_string(str);
            continue;
        }

      case state_e::quote:
        switch(*pos)
        {
          case '"':
            state = prev_state;
            continue;
          case '\\':
            switch(*++pos)
            {
              case 'a' : str.push_back('\a'); continue;
              case 'b' : str.push_back('\b'); continue;
              case 'f' : str.push_back('\f'); continue;
              case 'n' : str.push_back('\n'); continue;
              case 'r' : str.push_back('\r'); continue;
              case 't' : str.push_back('\t'); continue;
              case 'v' : str.push_back('\v'); continue;
              case '"' : str.push_back('"' ); continue;
              case '\\': str.push_back('\\'); continue;
              default: // unrecognized escape sequence!
                return bailout();
            }
          default:
            str.push_back(*pos);
            continue;
        }

      case state_e::comment:
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

bool write_node(std::shared_ptr<node_t> node, std::string section_name, std::multimap<std::string, std::string>& sections) noexcept
{
  auto section = sections.lower_bound(section_name);
  if(section == sections.end())
    section = sections.insert(section, std::make_pair(section_name, std::string()));

  for(const std::pair<std::string, std::shared_ptr<node_t>>& entry : node->values)
  {
    switch(entry.second->type)
    {
      case node_t::type_e::value:
        section->second += entry.first + '=' + entry.second->value + '\n';
        break;

      case node_t::type_e::array:
        section->second += '\n' + entry.first + '=';
        for(auto& valnode : entry.second->values)
          section->second += valnode.second->value + ','; // concatinate into a list
        section->second.pop_back(); // remove final ','
        section->second += '\n'; // add endline
        break;

      case node_t::type_e::multisection:
        if(!section->second.empty())
          sections.insert(section, std::make_pair(section_name, std::string()));
      case node_t::type_e::section:
        write_node(entry.second, section_name + '/' + entry.first, sections);
        break;

      default:
        return false;
    }
  }
  return true;
}

bool ConfigManip::write(std::string& data) const noexcept
{
  std::shared_ptr<node_t> node = *this;
  std::multimap<std::string, std::string> sections;
  if(!write_node(*this, "", sections))
    return false;
  for(const std::pair<std::string, std::string>& section : sections)
  {
    if(!section.second.empty())
    {
      if(section.first.size() > 1 && section.first.front() == '/')
        data += '[' + section.first.substr(1) + "]\n";
      data += section.second + '\n';
    }
  }
  return true;
}
