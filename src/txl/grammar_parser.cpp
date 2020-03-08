#include "grammar_parser.h"
#include <unordered_set>

using namespace tinyxml2;
using namespace TXL;


static unordered_set<string_view> const SPECIAL_SYMBOLS {
  NEW_LINE_TAG, "NL", "IN", "EX", "empty", "SPON", "SPOFF", "SP", "TAB", "TAB_16", "TAB_24", "!"
};

static unordered_set<string_view> const TYPE_MODIFIERS {
  "opt", "repeat", "list", "attr", "see", "not", "push", "pop"
};


XMLElement const* GrammarParser::expectedPath(XMLNode const* root,
                                              initializer_list<const char*> &&path) {
  XMLElement const* node = reinterpret_cast<XMLElement const*>(root);
  for (auto const p : path) {
    node = node->FirstChildElement(p);
    if (!node)
      throw p;
  }
  return node;
}

void GrammarParser::parseDefinition(XMLElement const* const definition) {
  auto const type_id = expectedPath(definition, { "typeid", "id" })->GetText();
  SCIS_DEBUG("Found definition of " << type_id);

  auto const type = grammar->findOrAddTypeByName(type_id);

  // process first template variant
  auto variant = &type->variants.emplace_back(/* empty */);
  parseLiteralsOrTypes(variant, expectedPath(definition, { "repeat_literalOrType" }));

  // process alternatives
  auto const alternativeVariants = definition->FirstChildElement("repeat_barLiteralsAndTypes");
  if (alternativeVariants)
    for (auto item = alternativeVariants->FirstChild(); item; item = item->NextSibling()) {
      variant = &type->variants.emplace_back(/* empty */);
      parseLiteralsOrTypes(variant, expectedPath(item, { "repeat_literalOrType" }));
    }
}

void GrammarParser::parseLiteralsOrTypes(TXLGrammar::TypeVariant* const typeVariant,
                                         XMLElement const* const literalsOrTypes) {
  for (auto item = literalsOrTypes->FirstChild(); item; item = item->NextSibling()) {
    auto const something = item->FirstChildElement();
    string_view const name = something->Name();
    if (name == "type") {
      // ignore formatting
      auto const type = something->FirstChildElement("typeSpec");
      if (type)
        parseType(typeVariant, type);
    }
    else
      if (name == "literal")
        parseLiteral(typeVariant, something);
      else
        throw "type> or <literal";
  }
}

void GrammarParser::parseType(TXLGrammar::TypeVariant* const typeVariant,
                              XMLElement const* const type) {
  auto const name = parseNameFromTypeid(expectedPath(type, { "typeid" }));

  if (isSpecial(name) || name.empty())
    return;

  // special case
  if (name.front() == '\'') {
    parseOptionalText(typeVariant, type, name);
    return;
  }

  auto typeRef = make_unique<TXLGrammar::TypeReference>();

  if (auto const o = type->FirstChildElement("opt_typeModifier"))
    typeRef->modifier = expectedPath(o, { "typeModifier" })->GetText();
  else
    typeRef->modifier = nullopt;

  typeRef->name = name;

  if (auto const o = type->FirstChildElement("opt_typeRepeater"))
    typeRef->repeater = expectedPath(o, { "typeRepeater" })->GetText();
  else
    typeRef->repeater = nullopt;

  typeVariant->pattern.push_back(std::move(typeRef));
}

string GrammarParser::parseNameFromTypeid(XMLElement const* const type_id) {
  auto const something = type_id->FirstChildElement();
  // try obvious
  if (!something)
    throw "id";

  string_view const name = something->Name();
  if (name == "id")
    return something->GetText();
  else
    if (name == "literal") {
      string text = "";
      mergeTextRecursive(text, something);
      return text;
    }
  else
    throw "literal";
}

void GrammarParser::parseLiteral(TXLGrammar::TypeVariant* const typeVariant,
                                 XMLElement const* const literal) {
  XMLElement const* unquotedLiteral = literal->FirstChildElement("unquotedLiteral");
  if (!unquotedLiteral)
    unquotedLiteral = literal->FirstChildElement("quotedLiteral");

  // is this a comment? - skip it
  if (!unquotedLiteral)
    if (expectedPath(literal, { "comment" }))
      return;

  string text = "";
  mergeTextRecursive(text, unquotedLiteral);

  auto txt = make_unique<TXLGrammar::PlainText>();
  txt->text = text;

  typeVariant->pattern.push_back(std::move(txt));
}

void GrammarParser::mergeTextRecursive(string &text,
                                       XMLNode const* const node) {
  if (auto const txt = node->ToText())
    text += txt->Value();
  else
    for (auto item = node->FirstChild(); item; item = item->NextSibling())
      mergeTextRecursive(text, item);
}

void GrammarParser::parseOptionalText(TXLGrammar::TypeVariant* const typeVariant,
                                      XMLElement const* const type,
                                      string_view const &text) {
  auto optText = make_unique<TXLGrammar::OptionalPlainText>();

  optText->modifier = expectedPath(type, { "opt_typeModifier", "typeModifier" })->GetText();
  optText->text = text;

  typeVariant->pattern.push_back(std::move(optText));
}

unique_ptr<TXLGrammar> GrammarParser::parse(XMLDocument const& doc) {
  grammar.reset(new TXLGrammar);

  try {
    auto const statements = expectedPath(&doc, { "program", "repeat_statement" });

    for (auto statement = statements->FirstChild(); statement; statement = statement->NextSibling())
      if (auto const definition = statement->FirstChildElement("defineStatement"))
        parseDefinition(definition);

    // skip everything else
  } catch (char const* const p) {
    SCIS_ERROR("Incorrect grammar. Cant find element <" << p << '>');
  }

  // additional check
  if (grammar->types.find("program") == grammar->types.cend())
    SCIS_WARNING("Type with name 'PROGRAM' not found in the grammar!");

  return std::move(grammar);
}

bool GrammarParser::isSpecial(string_view const& name) {
  return SPECIAL_SYMBOLS.find(name) != SPECIAL_SYMBOLS.cend();
}
