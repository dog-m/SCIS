#include "ruleset.h"
#include "logging.h"

using namespace std;
using namespace scis;

void BasicContext::dump(ostream &str)
{
  str << "{ ";
  auto count = constraints.size();
  for (auto const& c : constraints) {
    str << c.id << ' '
        << c.op << ' ';

    for (auto const& part : c.value)
      str << (part.somethingBefore ? "*" : "")
          << '\'' << part.text << '\''
          << (part.somethingAfter ? "*" : "");

    if (count --> 1)
      str << ", ";
  }
  str << " }";
}

void CompoundContext::dump(ostream &str)
{
  auto countAnd = references.size();
  for (auto const& disj : references) {
    str << '(';
    auto countOr = disj.size();
    for (auto const& ref : disj) {
      str << (ref.isNegative ? "!" : "") << ref.id;

      if (countOr --> 1)
        str << " | ";
    }
    str << ')';

    if (countAnd --> 1)
      str << " & ";
  }
}

void Rule::MakeAction::StringComponent::dump(ostream &str)
{
  str << text;
}

void Rule::MakeAction::StringComponent::toTXL(ostream& str)
{
  // WARNING: text already have quotes
  str << " [+ " << text << "]";
}

void Rule::MakeAction::ConstantComponent::dump(ostream &str)
{
  str << "$" + id;
}

void Rule::MakeAction::ConstantComponent::toTXL(ostream& str)
{
  string upcase = id;
  for (auto& c : upcase)
    c = toupper(c);

  str << " [quote " << upcase << "]";
}

void Ruleset::dump(ostream &str)
{
  str << "fragments:" << endl;
  for (auto const& f : fragments)
    str << "  " << f.path << endl;

  str << "contexts:" << endl;
  for (auto const& [_, f] : contexts) {
    str << "  " << f->id << ": ";
    f->dump(str);
    str << endl;
  }

  str << "rules:" << endl;
  for (auto const& [_, r] : rules) {
    str << "  " << r->id << ":" << endl;

    for (auto const& s : r->statements) {
      str << "    @" << s.location.contextId;
      for (auto const& p : s.location.path) {
        str << " -> ";

        str << '[' << p.modifier << "] " << p.keywordId;
        if (p.pattern.has_value())
          for (auto const& part : p.pattern.value())
            str << (part.somethingBefore ? "*" : "")
                << '\'' << part.text << '\''
                << (part.somethingAfter ? "*" : "");
      }
      str << " #" << s.location.pointcut << endl;

      str << (s.actionMake.empty() ? "" : "    make:\n");
      for (auto const& m : s.actionMake) {
        str << "      " << m.target << " <- ";

        auto countC = m.components.size();
        for (auto const& c : m.components) {
          c->dump(str);

          if (countC --> 1)
            str << " + ";
        }
        str << endl;
      }

      str << (s.actionAdd.empty() ? "" : "    add:") << endl;
      for (auto const& a : s.actionAdd) {
        str << "      " << a.fragmentId << '(';

        auto countA = a.args.size();
        for (auto const& c : a.args) {
          str << c;

          if (countA --> 1)
            str << ", ";
        }
        str << ')' << endl;
      }

      str << endl;
    }

    str << "----------" << endl;
  }
}
