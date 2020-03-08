#ifndef GRAMMARPARSER_H
#define GRAMMARPARSER_H

#include <vector>
#include <memory>

#include <tinyxml2/tinyxml2.h>

#include "grammar.h"
#include "interpreter.h"

namespace TXL {

  using namespace tinyxml2;

  class GrammarParser {
    unique_ptr<TXL::TXLGrammar> grammar;

    static XMLElement const* expectedPath(XMLNode const* root, initializer_list<const char*> && path);

    inline void parseDefinition(XMLElement const* const definition);

    void parseLiteralsOrTypes(TXLGrammar::TypeVariant* const typeVariant,
                              XMLElement const* const literalsOrTypes);

    void parseType(TXLGrammar::TypeVariant* const typeVariant,
                   XMLElement const* const type);

    string parseNameFromTypeid(XMLElement const* const type_id);

    void parseLiteral(TXLGrammar::TypeVariant* const typeVariant,
                      XMLElement const* const literal);

    void mergeTextRecursive(string &text,
                            XMLNode const* const node);

    void parseOptionalText(TXLGrammar::TypeVariant* const typeVariant,
                           XMLElement const* const type,
                           string_view const& text);

  public:
    unique_ptr<TXL::TXLGrammar> parse(XMLDocument const& doc);

    static bool isSpecial(string_view const& name);
  };

} // TXL namespace

#endif // GRAMMARPARSER_H
