#ifndef GRAMMARPARSER_H
#define GRAMMARPARSER_H

#include "grammar.h"

#include <vector>
#include <memory>
#include <unordered_set>

#include <tinyxml2/tinyxml2.h>

#include "interpreter.h"

namespace TXL {

  using namespace tinyxml2;

  class TXLGrammarParser {
    unique_ptr<TXLGrammar> grammar;

    void parse(XMLElement* node);

    /// build a pattern for a single variant
    void buildPatternForVariant(TXLGrammar::TypeVariant *const variant,
                                XMLNode *const firstChild);

  };

} // TXL namespace

#endif // GRAMMARPARSER_H
