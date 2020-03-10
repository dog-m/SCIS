#ifndef RULESET_PARSER_H
#define RULESET_PARSER_H

#include "ruleset.h"
#include <tinyxml2/tinyxml2.h>

#include "fragment_parser.h"

namespace SCIS {

  using namespace tinyxml2;

  class RulesetParser {
    unique_ptr<Ruleset> ruleset;

    void parseUsedFragments(XMLElement const* const fragments);

    void parseContexts(XMLElement const* const contexts);

    void parseRules(XMLElement const* const rules);

    void parseRuleStatements(Rule *const rule,
                             XMLElement const* const statements);

    void parseStatementLocation(Rule::Stetement & statement,
                                XMLElement const* const path);

    void parseStatementActions(Rule::Stetement & statement,
                               XMLElement const* const actions);

    void parseActions_Make(Rule::Stetement & statement,
                           XMLElement const* const makes);

    void parseActions_Make_singleComponent(Rule::MakeAction & make,
                                           XMLElement const* const stringlitOrConstant);

    void parseActions_Add(Rule::Stetement & statement,
                          XMLElement const* const additions);

  public:
    unique_ptr<Ruleset> parse(XMLDocument const& doc);

  }; // RulesetParser

} // SCIS

#endif // RULESET_PARSER_H
