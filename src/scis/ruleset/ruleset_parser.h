#ifndef RULESET_PARSER_H
#define RULESET_PARSER_H

#include "ruleset.h"
#include <tinyxml2/tinyxml2.h>

#include "fragment_parser.h"

namespace SCIS {

  using namespace tinyxml2;

  class RulesetParser {
    unique_ptr<Ruleset> ruleset;

    XMLElement const* expectedPath(XMLNode const* root,
                                   initializer_list<const char*> &&path);

    void parseUsedFragments(XMLElement const* const fragments);

    void parseContexts(XMLElement const* const contexts);

    void parseRules(XMLElement const* const rules);

  public:
    unique_ptr<Ruleset> parse(XMLDocument const& doc);

  }; // RulesetParser

}

#endif // RULESET_PARSER_H
