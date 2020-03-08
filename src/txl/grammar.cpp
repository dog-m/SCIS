#include "grammar.h"
#include "logging.h"

using namespace std;
using namespace TXL;

void TXLGrammar::PlainText::toTXL(ostream &ss) const {
  // BUG: potential formating bug here
  if (text.front() != '\'')
    ss << '\'';

  ss << text;
}

void TXLGrammar::TypeReference::toTXL(ostream &ss) const {
  ss << "["
     << (modifier.has_value() ? modifier.value() + " " : "")
     << name
     << repeater.value_or("")
     << "]";
}

void TXLGrammar::OptionalPlainText::toTXL(ostream &ss) const {
  ss << "[" << modifier << ' ' << text << "]";
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

void TXLGrammar::toDOT(ostream &ss) const {
  ss << "digraph G {" << endl;

  // pre-print all types
  for (auto const& [_, type] : types) {
    if (type->name != "program")
      ss << "  <" << type->name << "> ;" << endl;
    else
      ss << "  <program> [fillcolor=\"0.0 0.35 1.0\" style=filled];" << endl;
  }

  ss << endl;

  // print connections (ie references to other types)
  for (auto const& [_, type] : types) {
    ss << "  <" << type->name << "> -> { ";

    // get number of elements to print
    auto count = 0;
    for (auto const& v : type->variants)
      for (auto const& x : v.pattern)
        if (dynamic_cast<TXL::TXLGrammar::TypeReference*>(x.get()))
          ++count;

    // actual printing with commas
    for (auto const& v : type->variants)
      for (auto const& x : v.pattern)
        if (auto const ref = dynamic_cast<TXL::TXLGrammar::TypeReference*>(x.get())) {
          ss << "<" << ref->name << ">";

          if (count --> 1)
            ss << ", ";
        }

    ss << " };" << endl;
  }

  // finish graph
  ss << '}' << endl;
}
