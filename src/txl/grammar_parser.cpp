#include "grammar_parser.h"

using namespace tinyxml2;
using namespace TXL;

void TXLGrammarParser::parse(XMLElement *node) {
  string_view const typeName = node->Name();
  auto const type = grammar->getTypeByName(typeName);

  // add a new empty variant
  type->variants.emplace_back(/* empty */);
  auto const variant = type->variants.back().get();

  // build a pattern
  buildPatternForVariant(variant, node->FirstChild());
}

static bool isSpecial(string_view const& tagName) {
  static const unordered_set<string_view> specials {
    NEW_LINE_TAG, "NL", "IN", "EX", "empty", "SPON", "SPOFF"
  };
  return specials.find(tagName) != specials.cend();
}

void TXLGrammarParser::buildPatternForVariant(TXLGrammar::TypeVariant * const variant,
                                              XMLNode * const firstChild) {
  for (XMLNode* child = firstChild; child; child = child->NextSibling()) {
    if (auto const text = child->ToText()) {
      auto txt = make_unique<TXLGrammar::PlainText>();

      txt->text = text->Value();

      variant->pattern.push_back(std::move(txt));
    }
    else if (auto const tag = child->ToElement()) {
      string_view const tagName = tag->Name();

      if (isSpecial(tagName))
        continue;

      auto typeRef = make_unique<TXLGrammar::TypeReference>();

      auto const dashPos = tagName.find('_');
      if (dashPos != string_view::npos) {
        auto const prefix = tagName.substr(0, dashPos);
        auto const suffix = tagName.substr(dashPos + 1);
        typeRef->name = suffix;
        typeRef->function = prefix;
      }
      else {
        typeRef->name = tagName;
        typeRef->function = nullopt;
      }

      variant->pattern.push_back(std::move(typeRef));
    }
    else
      SCIS_ERROR("Unknown element in grammar: " << child->Value());
  }
}
