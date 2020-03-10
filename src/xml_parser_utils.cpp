#include "xml_parser_utils.h"

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

void mergeTextRecursive(string &text,
                        XMLNode const* const node)
{
  if (auto const txt = node->ToText())
    text += txt->Value();
  else
    FOREACH_XML_NODE(node, item)
      mergeTextRecursive(text, item);
}
