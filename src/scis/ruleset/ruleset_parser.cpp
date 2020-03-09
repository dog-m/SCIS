#include "ruleset_parser.h"
#include "logging.h"

using namespace std;
using namespace SCIS;
using namespace tinyxml2;

const XMLElement *RulesetParser::expectedPath(XMLNode const* root,
                                              initializer_list<const char *> &&path) {
  auto node = reinterpret_cast<XMLElement const*>(root);
  for (auto const p : path) {
    node = node->FirstChildElement(p);
    if (!node)
      throw p;
  }
  return node;
}

void RulesetParser::parseUsedFragments(XMLElement const *const fragments) {
  for (auto fragment = fragments->FirstChild(); fragment; fragment = fragment->NextSibling()) {
    auto const path = expectedPath(fragment, { "stringlit" })->GetText();
    auto& newFragment = ruleset->fragments.emplace_back(/* empty */);
    newFragment.path = path;
  }
}

void RulesetParser::parseContexts(const XMLElement * const contexts) {
  for (auto context = contexts->FirstChild(); context; context = context->NextSibling()) {
    ;// ???
  }
}

void RulesetParser::parseRules(const XMLElement * const rules) {
  for (auto rule = rules->FirstChild(); rule; rule = rule->NextSibling()) {
    ;// ???
  }
}

unique_ptr<Ruleset> RulesetParser::parse(XMLDocument const& doc) {
  ruleset.reset(new Ruleset);

  parseUsedFragments(expectedPath(&doc, { "program", "repeat_use_fragment_statement" }));
  parseContexts(expectedPath(&doc, { "program", "repeat_context_definition" }));
  parseRules(expectedPath(&doc, { "program", "rules", "repeat_single_rule" }));

  return std::move(ruleset);
}
