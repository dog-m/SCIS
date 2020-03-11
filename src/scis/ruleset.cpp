#include "ruleset.h"
#include "logging.h"

using namespace std;
using namespace scis;

string FragmentRequest::getSource(string const& rootDir) const
{
  return "["s + __PRETTY_FUNCTION__ + "] is not implemented"s;
}

void BasicContext::dump(ostream &str)
{
  str << "{ ";
  auto count = constraints.size();
  for (auto const& c : constraints) {
    str << c.id << ' '
        << c.op << ' '
        << (c.value.somethingBefore ? "..." : "") << c.value.text << (c.value.somethingAfter ? "..." : "");

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
      str << (ref.negation ? "!" : "") << ref.id;

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

void Rule::MakeAction::ConstantComponent::dump(ostream &str)
{
  str << "$" + id;
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

        str << '[' << p.modifier << "] " << p.statementId;
        if (p.pattern.has_value())
          str << " ("
               << (p.pattern->somethingBefore ? "..." : "")
               << p.pattern->text
               << (p.pattern->somethingAfter ? "..." : "")
               << ')';
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
