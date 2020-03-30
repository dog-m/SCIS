#ifndef GRAMMAR_PARSER_H
#define GRAMMAR_PARSER_H

#include <vector>
#include <memory>

#include <tinyxml2/tinyxml2.h>

#include "grammar.h"
#include "interpreter.h"

namespace txl {

  using namespace tinyxml2;

  class GrammarParser final {
    unique_ptr<txl::Grammar> grammar;

    inline void parseDefinition(XMLElement const* const definition);

    void parseLiteralsOrTypes(Grammar::TypeVariant* const typeVariant,
                              XMLElement const* const literalsOrTypes);

    void parseType(Grammar::TypeVariant* const typeVariant,
                   XMLElement const* const type);

    string parseNameFromTypeid(XMLElement const* const type_id);

    void parseLiteral(Grammar::TypeVariant* const typeVariant,
                      XMLElement const* const literal);

    static void mergeTextRecursiveNoComment(string &text,
                                            XMLNode const* const node);

    void parseOptionalText(Grammar::TypeVariant* const typeVariant,
                           XMLElement const* const type,
                           string_view const& text);

  public:
    unique_ptr<txl::Grammar> parse(XMLDocument const& doc);

    static bool isSpecial(string_view const& name);
  };

} // TXL

#endif // GRAMMAR_PARSER_H
