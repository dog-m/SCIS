#include "xml_parser_utils.h"
#include <sstream>

using namespace std;
using namespace tinyxml2;

XMLElement const* expectedPath(XMLNode const* root,
                               initializer_list<const char *> &&path)
{
  auto node = reinterpret_cast<XMLElement const*>(root);
  for (auto const p : path) {
    node = node->FirstChildElement(p);
    if (!node)
      throw "Cant find element <"s + p + ">"s;
  }
  return node;
}

XMLAttribute const* expectedAttribute(XMLElement const* xmlElement,
                                      const char *const attrName)
{
  auto const attr = xmlElement->FindAttribute(attrName);
  if (!attr)
    throw "Missing attribute \'"s + attrName + '\'';

  return attr;
}

void mergeTextRecursive(string &text,
                        XMLNode const* const node)
{
  if (auto const txt = node->ToText())
    text += txt->Value();
  else
    FOREACH_XML_NODE(node, item)
      mergeTextRecursive(text, item);
}

void processList(char const delimitter,
                 char const* const text,
                 std::function<void (string const&)> && handler)
{
  string str;
  std::stringstream stream(text);
  while( stream.good() ) {
      // skip spaces before useful data
      while (stream.peek() == ' ')
        stream.ignore();

      // grab data
      getline(stream, str, delimitter);

      // clear spaces after data
      while (!str.empty() && str.back() == ' ')
        str.pop_back();

      handler(str);
  }
}

void unescapeString(string& str)
{
  str.erase(std::remove(str.begin(), str.end(), '\"' ), str.end());
}

#include <iomanip>

string quote(string const& str)
{
  stringstream ss;
  ss << std::quoted(str);
  return ss.str();
}

string unquote(string const& str)
{
  string result;
  stringstream ss(str);
  ss >> std::quoted(result);
  return result;
}

string replace_all(string const& str, string const& a, string const& b)
{
  string result = str;

  size_t pos = 0;
  while((pos = result.find(a, pos)) != string::npos) {
    result.replace(pos, a.size(), b);
    pos += b.length();
  }

  return result;
}
