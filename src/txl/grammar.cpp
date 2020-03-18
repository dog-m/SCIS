#include "grammar.h"
#include "logging.h"

using namespace std;
using namespace txl;

void Grammar::PlainText::toTXL(ostream &ss, optional<NamingFunction> const) const
{
  // BUG: potential formating bug here
  if (!text.empty() && text.front() != '\'')
    ss << '\'';

  ss << text;
}

void Grammar::TypeReference::toTXL(ostream &ss, optional<NamingFunction> const namer) const
{
  auto const prefix = (modifier.has_value() ? modifier.value() + " " : "");

  if (namer.has_value())
    ss << namer.value()(prefix + name) << ' ';

  ss << "["
     << prefix
     << name
     << repeater.value_or("")
     << "]";
}

void Grammar::OptionalPlainText::toTXL(ostream &ss, optional<NamingFunction> const namer) const
{
  if (namer.has_value())
    ss << namer.value()(modifier + "_literal") << ' ';

  ss << "[" << modifier << ' ' << text << "]";
}

void Grammar::TypeVariant::toTXL(ostream &ss,
                                 size_t const baseIndent) const
{
  ss << string(baseIndent * 2, ' ');

  auto count = pattern.size();
  for (auto const& literal : pattern) {
    literal->toTXL(ss);

    // separate parts from each other
    if (count --> 1)
      ss << ' ';
  }
}

void Grammar::TypeVariant::toTXLWithNames(ostream& ss, const Grammar::NamingFunction namer) const
{
  ss << "    ";

  auto count = pattern.size();
  for (auto const& literal : pattern) {
    literal->toTXL(ss, namer);

    // separate parts from each other
    if (count --> 1)
      ss << ' ';
  }
}

void Grammar::Type::toTXL(ostream &ss,
                          size_t const baseIndent) const
{
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

void Grammar::Type::toTXLDefinition(ostream &ss) const
{
  ss << "define " << name << endl;

  toTXL(ss, 1);

  ss << endl << "end define" << endl << endl;
}

Grammar::Type* Grammar::findOrAddTypeByName(string_view const& name)
{
  if (auto const x = types.find(name); x != types.cend())
    return x->second.get();
  else {
    auto newType = make_unique<Type>();
    newType->name = name;
    auto const type = newType.get();

    types.insert_or_assign(newType->name, std::move(newType));

    return type;
  }
}

void Grammar::toDOT(ostream &ss) const
{
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
        if (dynamic_cast<txl::Grammar::TypeReference*>(x.get()))
          ++count;

    // actual printing with commas
    for (auto const& v : type->variants)
      for (auto const& x : v.pattern)
        if (auto const ref = dynamic_cast<txl::Grammar::TypeReference*>(x.get())) {
          ss << "<" << ref->name << ">";

          if (count --> 1)
            ss << ", ";
        }

    ss << " };" << endl;
  }

  // finish graph
  ss << '}' << endl;
}
