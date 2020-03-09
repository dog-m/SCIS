#include "ruleset_parser.h"
#include "logging.h"

using namespace std;
using namespace SCIS;
using namespace tinyxml2;

XMLElement const* RulesetParser::expectedPath(XMLNode const* root,
                                              initializer_list<const char *> &&path) {
  auto node = reinterpret_cast<XMLElement const*>(root);
  for (auto const p : path) {
    node = node->FirstChildElement(p);
    if (!node)
      throw p;
  }
  return node;
}

void RulesetParser::mergeTextRecursive(string &text,
                                       XMLNode const* const node) {
  if (auto const txt = node->ToText())
    text += txt->Value();
  else
    for (auto item = node->FirstChild(); item; item = item->NextSibling())
      mergeTextRecursive(text, item);
}

void RulesetParser::parseUsedFragments(XMLElement const *const fragments) {
  for (auto fragment = fragments->FirstChild(); fragment; fragment = fragment->NextSibling()) {
      auto const path = expectedPath(fragment, { "stringlit" })->GetText();
      auto& newFragment = ruleset->fragments.emplace_back(/* empty */);
    newFragment.path = path;

    SCIS_DEBUG("Found fragment request: " << path);
  }
}

void RulesetParser::parseContexts(const XMLElement * const contexts) {
  for (auto ctx = contexts->FirstChild(); ctx; ctx = ctx->NextSibling()) {
    ;// ???
  }
}

void RulesetParser::parseRules(const XMLElement * const rules) {
  for (auto singleRule = rules->FirstChild(); singleRule; singleRule = singleRule->NextSibling()) {
    auto rule = make_unique<Rule>();

    rule->id = expectedPath(singleRule, { "id" })->GetText();
    SCIS_DEBUG("Found rule " << rule->id);

    parseRuleStatements(rule.get(), expectedPath(singleRule, { "repeat_rule_statement" }));

    // check for duplications
    if (ruleset->rules.find(rule->id) != ruleset->rules.cend())
      SCIS_ERROR("Rule overwriting detected!");

    ruleset->rules.emplace(rule->id, std::move(rule));
  }
}

void RulesetParser::parseRuleStatements(Rule *const rule,
                                        XMLElement const* const statements) {
  for (auto singleStmt = statements->FirstChild(); singleStmt; singleStmt = singleStmt->NextSibling()) {
    auto& statement = rule->statements.emplace_back(/* empty */);
    parseStatementLocation(statement, expectedPath(singleStmt, { "rule_path" }));
    parseStatementActions(statement, expectedPath(singleStmt, { "rule_actions" }));
  }
}

void RulesetParser::parseStatementLocation(Rule::Stetement &statement,
                                           XMLElement const* const path) {
  // set context name
  string ctxId;
  mergeTextRecursive(ctxId, expectedPath(path, { "context_name" }));
  statement.location.contextId = ctxId;

  // set path
  auto const pathElements = expectedPath(path, { "repeat_path_item_with_arrow" });
  for (auto itemWithArrow = pathElements->FirstChild(); itemWithArrow; itemWithArrow = itemWithArrow->NextSibling()) {
    auto const item = expectedPath(itemWithArrow, { "path_item" });
    auto& el = statement.location.path.emplace_back(/* empty */);
    el.modifier = expectedPath(item, { "modifier", "id" })->GetText();
    el.statementId = expectedPath(item, { "statement_name", "id" })->GetText();

    if (auto const optTmp = item->FirstChildElement("opt_param_template")) {
      auto const tmp = expectedPath(optTmp, { "param_template", "param_template_template" });

      el.pattern = expectedPath(tmp, { "stringlit" })->GetText();

      tmp; // FIXME: add trailing dots (before)

      tmp; // FIXME: add trailing dots (after)
    }
    else
      el.pattern = nullopt;
  }

  // set pointcut name
  statement.location.pointcut = expectedPath(path, { "pointcut", "id" })->GetText();
}

void RulesetParser::parseStatementActions(Rule::Stetement &statement,
                                          XMLElement const* const actions) {
}

unique_ptr<Ruleset> RulesetParser::parse(XMLDocument const& doc) {
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
