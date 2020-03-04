#include "grammar.h"
#include "../logging.h"

using namespace std;
using namespace TXL;

void TXLGrammar::PlainText::toString(ostream &ss) const {
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

void TXLGrammar::TypeReference::toString(ostream &ss) const {
  ss << "[" << (function.has_value() ? function.value() + " " : "") << name << "]";
}

void TXLGrammar::TypeVariant::toString(ostream &ss) const {
  auto count = pattern.size();
  for (auto const& literal : pattern) {
    literal->toString(ss);

    // separate parts from each other
    if (count --> 1)
      ss << ' ';
  }
}

void TXLGrammar::Type::toString(ostream &ss) const {
  ss << "define " << name << endl;

  auto count = variants.size();
  for (auto const& var : variants) {
    ss << "  ";
    // render pattern variant as single row
    var.toString(ss);

    // separate variants by vertical line
    if (count --> 1)
      ss << endl << '|' << endl;
  }

  ss << endl << "end define" << endl << endl;
}

TXLGrammar::Type* TXLGrammar::getTypeByName(string_view const& name) {
  if (auto const x = types.find(name); x != types.cend())
    return x->second.get();
  else {
    auto const newType = new Type();
    newType->name = name;

    types.emplace(name, newType);

    return newType; // FIXME: !!!
  }
}
