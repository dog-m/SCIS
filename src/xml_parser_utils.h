#ifndef XML_PARSER_UTILS_H
#define XML_PARSER_UTILS_H

#include <string>
#include <functional>
#include <tinyxml2/tinyxml2.h>

#define FOREACH_XML_NODE(C, E) \
  for (auto E = C->FirstChild(); E; E = E->NextSibling())

#define FOREACH_XML_ELEMENT(C, E) \
  for (auto E = C->FirstChildElement(); E; E = E->NextSiblingElement())

#define FOREACH_XML_ATTRIBUTE(E, A) \
  for (auto A = E->FirstAttribute(); A; A = A->Next())

/// throws std::string
tinyxml2::XMLElement const* expectedPath(tinyxml2::XMLNode const* root,
                                         std::initializer_list<const char *> &&path);

/// throws std::string
tinyxml2::XMLAttribute const* expectedAttribute(tinyxml2::XMLElement const* xmlElement,
                                                const char *const attrName);

void mergeTextRecursive(std::string &text,
                        tinyxml2::XMLNode const* const node);

void processList(char const delimitter,
                 const char* const text,
                 std::function<void (std::string const&)> &&handler);

void unescapeString(std::string &str);

std::string quote(std::string const& str);

std::string unquote(std::string const& str);

std::string replace_all(std::string const& str,
                        std::string const& a,
                        std::string const& b);

#endif // XML_PARSER_UTILS_H
