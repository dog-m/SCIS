#include "grammar.h"
#include "../logging.h"

using namespace std;

void TXL::TXLGrammar::PlainText::toString(stringstream &ss) const {
  ss << text;
}

void TXL::TXLGrammar::TypeReference::toString(stringstream &ss) const {
  ss << "[" << (function.has_value() ? function.value() + " " : "") << name << "]";
}

void TXL::TXLGrammar::TypeVariant::toString(stringstream &ss) const {
  for (auto const& literal : pattern) {
    literal->toString(ss);

    // separate parts from each other
    ss << ' ';
  }
}

void TXL::TXLGrammar::Type::toString(stringstream &ss) const {
  auto count = variants.size();
  for (auto const& var : variants) {
    // render pattern variant as single row
    var->toString(ss);

    // separate variants by vertical line
    if (count --> 1)
      ss << endl << '|' << endl;
  }
}

TXL::TXLGrammar::Type *TXL::TXLGrammar::getTypeByName(const string_view &name) {
  if (auto const x = types.find(name); x != types.cend())
    return x->second.get();
  else {
    auto const newType = new Type();
    newType->name = name;
    types.emplace(name, newType);
    return newType; // FIXME: !!!
  }
}
