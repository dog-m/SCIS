#ifndef XML_PARSER_UTILS_H
#define XML_PARSER_UTILS_H

#include <string>
#include <functional>
#include <tinyxml2/tinyxml2.h>

#define FOREACH_XML_NODE(C, E) \
  for (auto E = C->FirstChild(); E; E = E->NextSibling())

#define FOREACH_XML_ELEMENT(C, E) \
  for (auto E = C->FirstChildElement(); E; E = E->NextSiblingElement())

/// throws std::string
tinyxml2::XMLElement const* expectedPath(tinyxml2::XMLNode const* root,
                                         std::initializer_list<const char *> &&path);

/// throws std::string
tinyxml2::XMLAttribute const* expectedAttribute(tinyxml2::XMLElement const* xmlElement,
                                                const char *const attrName);

void mergeTextRecursive(std::string &text,
                        tinyxml2::XMLNode const* const node);

void processCommaseparatedList(const char* const text,
                               std::function<void (std::string const&)> &&handler);

#endif // XML_PARSER_UTILS_H
