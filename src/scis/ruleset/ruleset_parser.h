#ifndef RULESET_PARSER_H
#define RULESET_PARSER_H

#include "ruleset.h"
#include <tinyxml2/tinyxml2.h>

#include "fragment_parser.h"

namespace SCIS {

  using namespace tinyxml2;

  class RulesetParser {
    unique_ptr<Ruleset> ruleset;

    static XMLElement const* expectedPath(XMLNode const* root,
                                          initializer_list<const char*> &&path);

    static void mergeTextRecursive(string &text,
                                   XMLNode const* const node);

    void parseUsedFragments(XMLElement const* const fragments);

    void parseContexts(XMLElement const* const contexts);

    void parseRules(XMLElement const* const rules);

    void parseRuleStatements(Rule *const rule,
                             XMLElement const* const statements);

    void parseStatementLocation(Rule::Stetement & statement,
                                XMLElement const* const path);

    void parseStatementActions(Rule::Stetement & statement,
                               XMLElement const* const actions);

    void parseActionMake(Rule::Stetement & statement,
                         XMLElement const* const actions);

    void parseActionAdd(Rule::Stetement & statement,
                        XMLElement const* const actions);

  public:
    unique_ptr<Ruleset> parse(XMLDocument const& doc);

  }; // RulesetParser

} // SCIS

#endif // RULESET_PARSER_H
