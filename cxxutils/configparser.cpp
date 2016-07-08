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

bool ConfigParser::parse(const std::string& strdata) noexcept
{
  enum state_e
  {
    searching = 0, // new line
    section,       // found section
    name,          // found name
    value,         // found value
    quote,         // found opening double quotation mark
    comment,       // found semicolon outside of quotation
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
            return bailout();

          case '#':
          case ';':
            prev_state = state;
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
            return bailout();

          case ';':
            prev_state = state;
            state = comment;
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
            state = searching;

            if(!node->value.empty()) // value name / section name conflict
              return bailout();

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
            state = value;
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
            state = comment;
            continue;
        }

      case value:
        switch(*pos)
        {
          default:
            if(!std::isspace(*pos) || !std::isspace(str.back())) // if not a space or string doesnt end with a space
              str.push_back(*pos);
            continue;

          case '=':
            return bailout();

          case '"':
            if(!str.empty()) // if quote is in the middle of a value rather than the start of it...
              return bailout();
            prev_state = state;
            state = quote;
            continue;

          case '\n':
            if(!str.empty())
            {
              if(!node->values.empty()) // if part of a list
                node = node->newChild(); // make new list entry
              node->value = use_string(str); // store value to current node
            }
            state = searching;
            continue;

          case ';':
            prev_state = state;
            state = comment;
            continue;

          case ',':
            node->newChild()->value = use_string(str);
            continue;
        }

      case quote:
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
