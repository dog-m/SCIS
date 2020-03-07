#include "grammar_parser.h"

using namespace tinyxml2;
using namespace TXL;

unique_ptr<TXLGrammar> parse_via_new_alternative_parser(XMLDocument const&);

static unordered_set<string_view> const SPECIAL_SYMBOLS {
  NEW_LINE_TAG, "NL", "IN", "EX", "empty", "SPON", "SPOFF", "SP", "TAB", "TAB_16", "TAB_24", "!"
};

static unordered_set<string_view> const TYPE_MODIFIERS {
  "opt", "repeat", "list", "attr", "see", "not", "push", "pop"
};

constexpr string_view TYPE_REPEATERS = "+*?,";

unique_ptr<TXLGrammar> TXLGrammarParser::parse(XMLDocument const& doc) {
  grammar.reset(new TXLGrammar);

  return parse_via_new_alternative_parser(doc);

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
  // skip special (definition only) types
  if (isSpecial(typeName))
    return;

  // skip refs
  if (bool const isReference = node->IntAttribute("ref", -1) > 0)
    // types itself will be added somewhere in the future on other branches (in queue)
    return;

  if (haveTypeModifier(typeName)) {
    // add subtree and quit
    for (auto child = node->FirstChild(); child; child = child->NextSibling())
      queue.push_back(child->ToElement());
    return;
  }

  // FIXME: [opt literal] and so on

  auto const type = grammar->findOrAddTypeByName(typeName);

  // build a pattern based on 'kind'
  string_view const kind = node->Attribute("kind");
  if (kind == "choose") {
    auto const addToPattern_kindChoose = [&](unique_ptr<TXLGrammar::Literal> && ptr) {
      type->variants.emplace_back(/* empty */);
      type->variants.back().pattern.push_back(std::move(ptr));
    };
    buildPatternForVariant(addToPattern_kindChoose, node->FirstChild(), typeName.data());
  }
  else {
    auto const addToPattern_kindOrder = [&](unique_ptr<TXLGrammar::Literal> && ptr) {
      type->variants.back().pattern.push_back(std::move(ptr));
    };
    // add a new empty variant
    type->variants.emplace_back(/* empty */);
    buildPatternForVariant(addToPattern_kindOrder, node->FirstChild(), nullptr);
  }
}

bool TXLGrammarParser::isSpecial(string_view const& name) {
  return SPECIAL_SYMBOLS.find(name) != SPECIAL_SYMBOLS.cend();
}

bool TXLGrammarParser::isTypeModifier(string_view const& name) {
  return TYPE_MODIFIERS.find(name) != TYPE_MODIFIERS.cend();
}

bool TXLGrammarParser::haveTypeRepeater(string_view const& name) {
  return TYPE_REPEATERS.find(name.back()) != string_view::npos;
}

bool TXLGrammarParser::haveTypeModifier(const string_view &name) {
  auto const pos = name.find('_');
  if (pos != string_view::npos) {
    auto const prefix = name.substr(0, pos);
    return isTypeModifier(prefix);
  }
  else
    return false;
}

void TXLGrammarParser::buildPatternForVariant(AddToPatternFunc const& addToPattern,
                                              XMLNode const* const firstChild,
                                              const char* const skipNodeWithName) {
  for (auto child = firstChild; child; child = child->NextSibling()) {
    if (auto const text = child->ToText()) {
      processText(addToPattern, text);
    }
    else if (auto const tag = child->ToElement()) {
      string_view const tagName = tag->Name();

      if (isSpecial(tagName))
        continue;

      // looking for something like [opt ';] ie <opt_literal ...>
      auto const dashPos = tagName.find('_');
      if (dashPos != string_view::npos &&
          isTypeModifier(tagName.substr(0, dashPos)) &&
          tagName.substr(dashPos + 1) == "literal") {
        //
        processLiteral(addToPattern, tag);
      }
      else {
        // add child to queue (BFS)
        queue.push_back(tag);

        if (tagName == skipNodeWithName)
          continue;

        processElement(addToPattern, tag);
      }
    }
    else
      SCIS_ERROR("Unknown element in grammar: " << child->Value());
    }
}

void TXLGrammarParser::processText(TXLGrammarParser::AddToPatternFunc const& addToPattern,
                                   XMLText const* const child) {
  auto txt = make_unique<TXLGrammar::PlainText>();

  txt->text = child->Value();

  addToPattern(std::move(txt));
}

void TXLGrammarParser::processElement(TXLGrammarParser::AddToPatternFunc const& addToPattern,
                                      XMLElement const* const child) {
  string_view const tagName = child->Name();
  auto typeRef = make_unique<TXLGrammar::TypeReference>();

  typeRef->modifier = nullopt;
  typeRef->name = tagName;
  typeRef->repeater = nullopt;

  // extract modifier
  auto const dashPos = tagName.find('_');
  if (dashPos != string_view::npos) {
    auto const prefix = tagName.substr(0, dashPos);

    // is it a special function?
    if (isTypeModifier(prefix)) {
      auto const suffix = tagName.substr(dashPos + 1);
      typeRef->modifier = prefix;
      typeRef->name = suffix;
    }
  }

  // extract repeater (highly depends on order!)
  auto const repeatPos = typeRef->name.find_first_of(TYPE_REPEATERS);
  if (repeatPos != string::npos) {
    typeRef->repeater = typeRef->name.substr(repeatPos);
    typeRef->name = typeRef->name.substr(0, repeatPos);
  }

  addToPattern(std::move(typeRef));
}

void TXLGrammarParser::processLiteral(TXLGrammarParser::AddToPatternFunc const& addToPattern,
                                      XMLElement const* const tag) {
  //auto lit = make_unique<TXLGrammar::OptionalPlainText>();
  processElement(addToPattern, tag);
}






struct AlternativeParser {

  unique_ptr<TXL::TXLGrammar> grammar;

  XMLElement const* safePath(XMLNode const* root, vector<string_view> && path) const {
    XMLElement const* node = reinterpret_cast<XMLElement const*>(root);
    for (auto const& p : path) {
      node = node->FirstChildElement(p.data());
      if (!node)
        throw p;
    }
    return node;
  }

  unique_ptr<TXL::TXLGrammar> parse(XMLDocument const& doc) {
    grammar.reset(new TXLGrammar);

    try {
      auto const statements = safePath(&doc, { "program", "repeat_statement" });
      ;//
      cout << statements << endl;
    } catch (string_view const& p) {
      SCIS_ERROR("Incorrect grammar. Cant find element with name = " << p);
    }

    return std::move(grammar);
  }

};

unique_ptr<TXLGrammar> parse_via_new_alternative_parser(XMLDocument const& doc) {
  AlternativeParser p;
  return p.parse(doc);
}
