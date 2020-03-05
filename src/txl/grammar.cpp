#include "grammar.h"
#include "../logging.h"

using namespace std;
using namespace TXL;

void TXLGrammar::PlainText::toTXL(ostream &ss) const {
  bool showQuote = true;

  for (auto const c : text) {
    // escape terminal symbols
    if (showQuote) {
      ss << '\'';
      showQuote = false;
    }
    else if (c == ' ' || c == '\t')
      showQuote = true;

    ss << c;
  }
}

void TXLGrammar::TypeReference::toTXL(ostream &ss) const {
  ss << "["
     << (function.has_value() ? function.value() + " " : "")
     << name
     << "]";
}

void TXLGrammar::TypeVariant::toTXL(ostream &ss, size_t const baseIndent) const {
  ss << string(baseIndent * 2, ' ');

  auto count = pattern.size();
  for (auto const& literal : pattern) {
    literal->toTXL(ss);

    // separate parts from each other
    if (count --> 1)
      ss << ' ';
  }
}

void TXLGrammar::Type::toTXL(ostream &ss, size_t const baseIndent) const {
  auto const indent = string(baseIndent * 2, ' ');

  auto count = variants.size();
  for (auto const& var : variants) {
    // render pattern variant as single row
    var.toTXL(ss, baseIndent + 1);

    // separate variants by vertical line
    if (count --> 1)
      ss << endl
         << indent << '|' << endl;
  }
}

void TXLGrammar::Type::toTXLDefinition(ostream &ss) const {
  ss << "define " << name << endl;

  toTXL(ss, 1);

  ss << endl << "end define" << endl << endl;
}

TXLGrammar::Type* TXLGrammar::findOrAddTypeByName(string_view const& name) {
  if (auto const x = types.find(name); x != types.cend())
    return x->second.get();
  else {
    auto const newType = new Type();
    newType->name = name;

    types.emplace(name, newType);

    return newType;
  }
}
