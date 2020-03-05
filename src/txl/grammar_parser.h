#ifndef GRAMMARPARSER_H
#define GRAMMARPARSER_H

#include <vector>
#include <memory>
#include <unordered_set>
#include <deque>
#include <functional>

#include <tinyxml2/tinyxml2.h>

#include "grammar.h"
#include "interpreter.h"

namespace TXL {

  using namespace tinyxml2;

  class TXLGrammarParser {
    unique_ptr<TXLGrammar> grammar;
    deque<XMLElement const*> queue;

    void parseElement(XMLElement const* const node);

    using AddToPatternFunc = function<void(unique_ptr<TXLGrammar::Literal>&&)>;

    /// build a pattern for a single variant
    void buildPatternForVariant(AddToPatternFunc const& addToPattern,
                                XMLNode const* const firstChild,
                                const char* const skipNodeWithName);

    void processText(AddToPatternFunc const& addToPattern,
                     XMLText const* const child);

    void processElement(AddToPatternFunc const& addToPattern,
                        XMLElement const* const child);

    void processLiteral(AddToPatternFunc const& addToPattern,
                        XMLElement const* const tag);

  public:
    unique_ptr<TXLGrammar> parse(XMLDocument const& doc);

    static bool isSpecial(string_view const& name);
    static bool isTypeModifier(string_view const& name);
    static bool haveTypeModifier(string_view const& name);
    static bool haveTypeRepeater(string_view const& name);
  };

} // TXL namespace

#endif // GRAMMARPARSER_H
