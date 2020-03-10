#ifndef XML_PARSER_UTILS_H
#define XML_PARSER_UTILS_H

#include <string>
#include <tinyxml2/tinyxml2.h>

#ifndef FOREACH_XML_NODE
#define FOREACH_XML_NODE(C, E) \
  for (auto E = C->FirstChild(); E; E = E->NextSibling())
#endif

#ifndef FOREACH_XML_ELEMENT
#define FOREACH_XML_ELEMENT(C, E) \
  for (auto E = C->FirstChildElement(); E; E = E->NextSiblingElement())
#endif

tinyxml2::XMLElement const* expectedPath(tinyxml2::XMLNode const* root,
                                         std::initializer_list<const char *> &&path);

void mergeTextRecursive(std::string &text,
                        tinyxml2::XMLNode const* const node);

#endif // XML_PARSER_UTILS_H
