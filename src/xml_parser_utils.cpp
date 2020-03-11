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

void processCommaseparatedList(const char * const text,
                               std::function<void (string const&)> && handler)
{
  std::stringstream stream(text);
  for (std::string str; stream >> str;) {
    handler(str);

    // skip delimitters
    while (stream.peek() == ',' || stream.peek() == ' ')
      stream.ignore();
  }
}
