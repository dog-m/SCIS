#include "ruleset_parser.h"
#include "logging.h"

using namespace std;
using namespace scis;
using namespace tinyxml2;

#include "../xml_parser_utils.h"

void RulesetParser::parseStringTemplate(Pattern &pattern,
                                        XMLElement const* const core)
{
  pattern.somethingBefore = core->PreviousSiblingElement("opt_literal");
  pattern.text = core->GetText();
  pattern.somethingAfter = core->NextSiblingElement("opt_literal");
}

void RulesetParser::parseUsedFragments(XMLElement const* const fragments)
{
  FOREACH_XML_ELEMENT(fragments, fragment) {
    auto const path = expectedPath(fragment, { "stringlit" })->GetText();
    auto& newFragment = ruleset->fragments.emplace_back(/* empty */);
    newFragment.path = path;

    SCIS_DEBUG("Found fragment request: " << path);
  }
}

void RulesetParser::parseContexts(XMLElement const* const contexts)
{
  FOREACH_XML_ELEMENT(contexts, ctx) {
    auto const id = expectedPath(ctx, { "context_name", "id" })->GetText();
    auto const something = expectedPath(ctx, { "basic_context_or_compound_context" })->FirstChildElement();
    SCIS_DEBUG("Found context \'" << id << '\'');

    if (something->Name() == "basic_context"sv)
      parseBasicContext(id, something);
    else
      if (something->Name() == "compound_context"sv)
        parseCompoundContext(id, something);
    else
      throw "Unknown context type. ID = "s + id;
  }
}

void RulesetParser::parseBasicContext(string const& id,
                                      XMLElement const* const basic_context)
{
  auto ctx = make_unique<BasicContext>();
  ctx->id = id;

  auto const list = expectedPath(basic_context, { "list_basic_context_constraint" });
  FOREACH_XML_ELEMENT(list, item) {
    auto& constraint = ctx->constraints.emplace_back(/* empty */);

    auto const property = expectedPath(item, { "context_property" });
    mergeTextRecursive(constraint.id, property);

    if (constraint.id.empty())
      throw "Incorrect ID in constraints of context [" + id + "]";

    constraint.op = expectedPath(item, { "context_op" })->GetText();

    parseStringTemplate(constraint.value, expectedPath(item, { "string_template", "stringlit" }));
  }

  ruleset->contexts.emplace(id, std::move(ctx));
}

void RulesetParser::parseCompoundContext(string const& id,
                                         XMLElement const* const compound_context)
{
  auto ctx = make_unique<CompoundContext>();
  ctx->id = id;

  auto const chain = expectedPath(compound_context, { "cnf_entry", "cnf_expression_lists", "repeat_cnf_expression_chain_and" });
  FOREACH_XML_ELEMENT(chain, element) {
    auto& disjunction = ctx->references.emplace_back(/* empty */);
    auto const chain_element = expectedPath(element, { "cnf_expression_chain_or" });
    parseContextDisjunction(disjunction, chain_element);
  }

  // TODO: check if every context (id) exists in compound context

  ruleset->contexts.emplace(id, std::move(ctx));
}

void RulesetParser::parseContextDisjunction(CompoundContext::Disjunction &disjunction,
                                            XMLElement const* const chain_element)
{
  auto const something = chain_element->FirstChildElement();

  // is it a single element?
  if (something->Name() == "cnf_expression_element"sv) {
    auto& ref = disjunction.emplace_back(/* empty */);
    ref.negation = something->FirstChildElement("opt_literal");
    mergeTextRecursive(ref.id, expectedPath(something, { "cnf_var_or_const" }));
  }
  else
    // is it a sequence?
    if (something->Name() == "repeat_cnf_expression_element_chain"sv) {
      FOREACH_XML_ELEMENT(something, element)
        parseContextDisjunction(disjunction, element);
    }
  else
    throw "Unrecognized context disjunction element "s + (something ? something->Name() : "NULL");
}

void RulesetParser::parseRules(XMLElement const* const rules)
{
  FOREACH_XML_ELEMENT(rules, singleRule) {
    auto rule = make_unique<Rule>();

    rule->id = expectedPath(singleRule, { "id" })->GetText();
    SCIS_DEBUG("Found rule \'" << rule->id << '\'');

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
  FOREACH_XML_ELEMENT(statements, singleStmt) {
    auto& statement = rule->statements.emplace_back(/* empty */);
    parseStatementLocation(statement, expectedPath(singleStmt, { "rule_path" }));
    parseStatementActions(statement, expectedPath(singleStmt, { "rule_actions" }));
  }
}

void RulesetParser::parseStatementLocation(Rule::Statement &statement,
                                           XMLElement const* const path)
{
  // set context name
  string ctxId;
  mergeTextRecursive(ctxId, expectedPath(path, { "context_name" }));
  statement.location.contextId = ctxId;

  // set path
  auto const pathElements = expectedPath(path, { "repeat_path_item_with_arrow" });
  FOREACH_XML_ELEMENT(pathElements, itemWithArrow) {
    auto const item = expectedPath(itemWithArrow, { "path_item" });
    auto& el = statement.location.path.emplace_back(/* empty */);
    el.modifier = expectedPath(item, { "modifier", "id" })->GetText();
    el.statementId = expectedPath(item, { "statement_name", "id" })->GetText();

    if (auto const optTmp = item->FirstChildElement("opt_param_template")) {
      // actual string
      auto& pattern = el.pattern = Pattern();
      parseStringTemplate(*pattern, expectedPath(optTmp, { "param_template", "string_template", "stringlit" }));
    }
    else
      el.pattern = nullopt;
  }

  // set pointcut name
  statement.location.pointcut = expectedPath(path, { "pointcut", "id" })->GetText();
}

void RulesetParser::parseStatementActions(Rule::Statement &statement,
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

void RulesetParser::parseActions_Make(Rule::Statement &statement,
                                      XMLElement const* const makes)
{
  FOREACH_XML_ELEMENT(makes, makeItem) {
    auto& make = statement.actionMake.emplace_back(/* empty */);
    make.target = expectedPath(makeItem, { "id" })->GetText();

    // process first element
    auto const chainHead = expectedPath(makeItem, { "string_chain", "stringlit_or_constant" });
    parseActions_Make_singleComponent(make, chainHead);

    // and other elements
    FOREACH_XML_ELEMENT(chainHead->NextSibling(), ss)
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
    throw "Unrecognized type of element found when parsing 'MAKE' action: "s + something->Value();
}

void RulesetParser::parseActions_Add(Rule::Statement &statement,
                                     XMLElement const* const additions)
{
  FOREACH_XML_ELEMENT(additions, add) {
    auto& act = statement.actionAdd.emplace_back(/* empty */);
    act.fragmentId = expectedPath(add, { "id" })->GetText();

    // TODO: check ID in imported fragments

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
    // fragments are required
    parseUsedFragments(expectedPath(&doc, { "program", "repeat_use_fragment_statement" }));

    // contexts are optional
    auto const contexts = doc.RootElement()->FirstChildElement("repeat_context_definition");
    if (contexts)
      parseContexts(contexts);

    // rules are required
    parseRules(expectedPath(&doc, { "program", "rules", "repeat_single_rule" }));

    // skip everything else
  } catch (string const msg) {
    SCIS_ERROR("Incorrect ruleset. " << msg);
  }

  return std::move(ruleset);
}
