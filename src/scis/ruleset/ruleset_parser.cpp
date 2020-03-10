#include "ruleset_parser.h"
#include "logging.h"

using namespace std;
using namespace SCIS;
using namespace tinyxml2;

#include "../xml_parser_utils.h"

void RulesetParser::parseUsedFragments(XMLElement const* const fragments)
{
  FOREACH_XML_NODE(fragments, fragment) {
    auto const path = expectedPath(fragment, { "stringlit" })->GetText();
    auto& newFragment = ruleset->fragments.emplace_back(/* empty */);
    newFragment.path = path;

    SCIS_DEBUG("Found fragment request: " << path);
  }
}

void RulesetParser::parseContexts(XMLElement const* const contexts)
{
  FOREACH_XML_NODE(contexts, ctx) {
    ;// TODO: contexts
  }
}

void RulesetParser::parseRules(XMLElement const* const rules)
{
  FOREACH_XML_NODE(rules, singleRule) {
    auto rule = make_unique<Rule>();

    rule->id = expectedPath(singleRule, { "id" })->GetText();
    SCIS_DEBUG("Found rule " << rule->id);

    parseRuleStatements(rule.get(), expectedPath(singleRule, { "repeat_rule_statement" }));

    // check for duplications
    if (ruleset->rules.find(rule->id) != ruleset->rules.cend())
      SCIS_WARNING("Rule overwriting detected!");

    ruleset->rules.emplace(rule->id, std::move(rule));
  }
}

void RulesetParser::parseRuleStatements(Rule *const rule,
                                        XMLElement const* const statements)
{
  FOREACH_XML_NODE(statements, singleStmt) {
    auto& statement = rule->statements.emplace_back(/* empty */);
    parseStatementLocation(statement, expectedPath(singleStmt, { "rule_path" }));
    parseStatementActions(statement, expectedPath(singleStmt, { "rule_actions" }));
  }
}

void RulesetParser::parseStatementLocation(Rule::Stetement &statement,
                                           XMLElement const* const path)
{
  // set context name
  string ctxId;
  mergeTextRecursive(ctxId, expectedPath(path, { "context_name" }));
  statement.location.contextId = ctxId;

  // set path
  auto const pathElements = expectedPath(path, { "repeat_path_item_with_arrow" });
  FOREACH_XML_NODE(pathElements, itemWithArrow) {
    auto const item = expectedPath(itemWithArrow, { "path_item" });
    auto& el = statement.location.path.emplace_back(/* empty */);
    el.modifier = expectedPath(item, { "modifier", "id" })->GetText();
    el.statementId = expectedPath(item, { "statement_name", "id" })->GetText();

    if (auto const optTmp = item->FirstChildElement("opt_param_template")) {
      // actual string
      auto const patternStr = expectedPath(optTmp, { "param_template", "param_template_template", "stringlit" });
      el.pattern.text = patternStr->GetText();

      // prefixes and suffixes
      el.pattern.somethingBefore = patternStr->PreviousSiblingElement("opt_literal");
      el.pattern.somethingAfter = patternStr->NextSiblingElement("opt_literal");
    }
    else
      el.pattern.text = nullopt;
  }

  // set pointcut name
  statement.location.pointcut = expectedPath(path, { "pointcut", "id" })->GetText();
}

void RulesetParser::parseStatementActions(Rule::Stetement &statement,
                                          XMLElement const* const actions)
{
  // parse "make" action if presented
  if (auto const optMake = actions->FirstChildElement("opt_action_make")) {
    auto const makes = expectedPath(optMake, { "action_make", "repeat_action_make_item" });
    parseActions_Make(statement, makes);
  }

  // parse "add" set of actions
  parseActions_Add(statement, expectedPath(actions, { "action_add", "list_action_id" }));
}

void RulesetParser::parseActions_Make(Rule::Stetement &statement,
                                      XMLElement const* const makes)
{
  FOREACH_XML_NODE(makes, makeItem) {
    auto& make = statement.actionMake.emplace_back(/* empty */);
    make.target = expectedPath(makeItem, { "id" })->GetText();

    // process first element
    auto const chainHead = expectedPath(makeItem, { "string_chain", "stringlit_or_constant" });
    parseActions_Make_singleComponent(make, chainHead);

    // and other elements
    FOREACH_XML_NODE(chainHead->NextSibling(), ss)
      parseActions_Make_singleComponent(make, expectedPath(ss, { "stringlit_or_constant" }));
  }
}

void RulesetParser::parseActions_Make_singleComponent(Rule::MakeAction &make,
                                                      XMLElement const* const stringlitOrConstant)
{
  auto const something = stringlitOrConstant->FirstChildElement();
  if (something->Name() == "string_constant"sv) {
    auto ptr = make_unique<Rule::MakeAction::ConstantComponent>();
    ptr->id = expectedPath(something, { "id" })->GetText();
    make.components.emplace_back(std::move(ptr));
  }
  else
    if (something->Name() == "stringlit"sv) {
      auto ptr = make_unique<Rule::MakeAction::StringComponent>();
      ptr->text = something->GetText();
      make.components.emplace_back(std::move(ptr));
    }
  else
    throw "?>. Unrecognized type of element found when parsing 'MAKE' action";
}

void RulesetParser::parseActions_Add(Rule::Stetement &statement,
                                     XMLElement const* const additions)
{
  FOREACH_XML_NODE(additions, add) {
    auto& act = statement.actionAdd.emplace_back(/* empty */);
    act.fragmentId = expectedPath(add, { "id" })->GetText();

    // FIXME: check ID in imported fragments

    auto const optArgs = add->FirstChildElement("opt_template_args");
    if (optArgs) {
      auto const args = expectedPath(optArgs, { "template_args", "list_id" });
      FOREACH_XML_ELEMENT(args, arg)
        act.args.push_back(arg->GetText());
    }
  }
}

unique_ptr<Ruleset> RulesetParser::parse(XMLDocument const& doc)
{
  ruleset.reset(new Ruleset);

  try {
    parseUsedFragments(expectedPath(&doc, { "program", "repeat_use_fragment_statement" }));
    parseContexts(expectedPath(&doc, { "program", "repeat_context_definition" }));
    parseRules(expectedPath(&doc, { "program", "rules", "repeat_single_rule" }));

    // skip everything else
  } catch (char const* const p) {
    SCIS_ERROR("Incorrect ruleset. Cant find element <" << p << '>');
  }

  return std::move(ruleset);
}
