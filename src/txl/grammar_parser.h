#ifndef GRAMMARPARSER_H
#define GRAMMARPARSER_H

#include "grammar.h"

#include <vector>
#include <memory>
#include <unordered_set>
#include <deque>

#include <tinyxml2/tinyxml2.h>

#include "interpreter.h"

namespace TXL {

  using namespace tinyxml2;

  class TXLGrammarParser {
    unique_ptr<TXLGrammar> grammar;
    deque<XMLElement const*> queue;

    void parseElement(XMLElement const* const node);

    /// build a pattern for a single variant
    void buildPatternForVariant(TXLGrammar::TypeVariant *const variant,
                                XMLNode const* const firstChild);

  public:
    unique_ptr<TXLGrammar> parse(XMLDocument const& doc);

    static bool isSpecial(string_view const& name);
    static bool isInternalFunction(string_view const& name);
    static bool hasInternalFunction(string_view const& name);
  };

} // TXL namespace

#endif // GRAMMARPARSER_H
