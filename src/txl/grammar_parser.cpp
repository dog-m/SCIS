#include "grammar_parser.h"

using namespace tinyxml2;
using namespace TXL;

unique_ptr<TXLGrammar> TXLGrammarParser::parse(XMLDocument const& doc) {
  grammar.reset(new TXLGrammar);

  queue.push_back(doc.RootElement());
  // BFS
  while (!queue.empty()) {
    auto const node = queue.front();
    queue.pop_front();

    parseElement(node);
  }

  // additional check
  if (grammar->types.find("program") == grammar->types.cend())
    SCIS_ERROR("Type with name 'PROGRAM' not found in the grammar!");

  return std::move(grammar);
}

void TXLGrammarParser::parseElement(XMLElement const *const node) {
  string_view const typeName = node->Name();
  bool const isReference = node->IntAttribute("ref", -1) > 0;

  // skip refs
  if (isReference)
    // types itself will be added somewhere in the future on other branches (in queue)
    return;

  if (hasInternalFunction(typeName)) {
    // add subtree and quit
    for (auto child = node->FirstChild(); child; child = child->NextSibling())
      queue.push_back(child->ToElement());
    return;
  }

  auto const type = grammar->getTypeByName(typeName);
  // add a new empty variant
  type->variants.emplace_back(/* empty */);
  auto const variant = &type->variants.back();

  // build a pattern
  buildPatternForVariant(variant, node->FirstChild());
}

bool TXLGrammarParser::isSpecial(string_view const& name) {
  static unordered_set<string_view> const specials {
    NEW_LINE_TAG, "NL", "IN", "EX", "empty", "SPON", "SPOFF"
  };
  return specials.find(name) != specials.cend();
}

bool TXLGrammarParser::isInternalFunction(string_view const& name) {
  static unordered_set<string_view> const functions {
    "opt", "list", "repeat"
  };
  return functions.find(name) != functions.cend();
}

bool TXLGrammarParser::hasInternalFunction(const string_view &name) {
  auto const pos = name.find('_');
  if (pos != string_view::npos) {
    auto const prefix = name.substr(0, pos);
    return isInternalFunction(prefix);
  }
  else
    return false;
}

void TXLGrammarParser::buildPatternForVariant(TXLGrammar::TypeVariant * const variant,
                                              XMLNode const* const firstChild) {
  for (auto child = firstChild; child; child = child->NextSibling()) {
    if (auto const text = child->ToText()) {
      auto txt = make_unique<TXLGrammar::PlainText>();

      txt->text = text->Value();
      auto const x = txt->text.find_first_not_of(" \t");
      txt->text.erase(0, x);

      variant->pattern.push_back(std::move(txt));
    }
    else if (auto const tag = child->ToElement()) {
      string_view const tagName = tag->Name();

      if (isSpecial(tagName))
        continue;

      // add child to queue (BFS)
      queue.push_back(tag);

      auto typeRef = make_unique<TXLGrammar::TypeReference>();

      typeRef->name = tagName;
      typeRef->function = nullopt;

      auto const dashPos = tagName.find('_');
      if (dashPos != string_view::npos) {
        auto const prefix = tagName.substr(0, dashPos);

        // is it a special function?
        if (isInternalFunction(prefix)) {
          auto const suffix = tagName.substr(dashPos + 1);
          typeRef->name = suffix;
          typeRef->function = prefix;
        }
      }

      variant->pattern.push_back(std::move(typeRef));
    }
    else
      SCIS_ERROR("Unknown element in grammar: " << child->Value());
  }
}
