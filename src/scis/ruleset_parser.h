#ifndef RULESET_PARSER_H
#define RULESET_PARSER_H

#include "ruleset.h"
#include <tinyxml2/tinyxml2.h>

namespace scis {

  using namespace tinyxml2;

  class RulesetParser final {
    unique_ptr<Ruleset> ruleset;

    /// core = [stringlit] node in grammar
    static void parseStringTemplate(Pattern &pattern,
                                    XMLElement const* const core);

    void parseUsedFragments(XMLElement const* const fragments);

    void parseContexts(XMLElement const* const contexts);

    void parseBasicContext(string const& id,
                           XMLElement const* const basic_context);

    void parseCompoundContext(string const& id,
                              XMLElement const* const compound_context);

    void parseContextDisjunction(CompoundContext::Disjunction & disjunction,
                                 XMLElement const* const chain_element);

    void parseRules(XMLElement const* const rules);

    void parseRuleStatements(Rule *const rule,
                             XMLElement const* const statements);

    void parseStatementLocation(Rule::Statement & statement,
                                XMLElement const* const path);

    void parseStatementActions(Rule::Statement & statement,
                               XMLElement const* const actions);

    void parseActions_Make(Rule::Statement & statement,
                           XMLElement const* const makes);

    void parseActions_Make_singleComponent(Rule::MakeAction & make,
                                           XMLElement const* const stringlitOrConstant);

    void parseActions_Add(Rule::Statement & statement,
                          XMLElement const* const additions);

  public:
    unique_ptr<Ruleset> parse(XMLDocument const& doc);

  }; // RulesetParser

} // SCIS

#endif // RULESET_PARSER_H
