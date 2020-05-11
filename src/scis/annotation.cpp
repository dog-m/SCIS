#include "annotation.h"
#include "logging.h"

using namespace std;
using namespace scis;

void GrammarAnnotation::dump(ostream& str)
{
  str << "pipeline: " << pipeline << endl
      << "Grammar:" << endl
      << "  lang = "          << grammar.language << endl
      << "  src = "           << grammar.txlSourceFilename << endl
      << "  user-var-type = " << grammar.userVariableType << endl
      << "DAG:" << endl;

  for (auto const& [_, keyword] : grammar.graph.keywords) {
    str << "  " << keyword->id << " : [" << keyword->type << "]" << endl;

    str << "  containing keywords: ";
    auto countR = keyword->subnodes.size();
    for (auto const& ref : keyword->subnodes) {
      str << ref;

      if (countR --> 1)
        str << ", ";
    }
    str << endl;

    auto countT = keyword->templates.size();
    str << "  templates (" << countT << "):" << endl;
    for (auto const& [_, iTemplate] : keyword->templates) {
      for (auto const& block : iTemplate->blocks)
        block->dump(str);

      if (countT --> 1)
        str << "  ....." << endl;
    }
    
    str << "  pointcuts (" << keyword->pointcuts.size() << "):" << endl;
    for (auto const& [_, point] : keyword->pointcuts) {
      str << "    name: \'" << point->name << '\'' << endl;

      str << "    algorithm (" << point->aglorithm.size() << " steps):" << endl;
      for (auto const& x : point->aglorithm) {
        str << "      " << x.function << " (";
        auto countA = x.args.size();
        for (auto const& [name, value] : x.args) {
          str << name << "=\'" << value << '\'';

          if (countA --> 1)
            str << ", ";
        }
        str << ')' << endl;
      }
    }

    str << "---" << endl;
  }
}

void GrammarAnnotation::Template::TextBlock::dump(ostream& str) const
{
  str << "    ::text::\t" << text << endl;
}

void GrammarAnnotation::Template::TextBlock::toTXLWithNames(
    ostream& str,
    NamingFunction const&) const
{
  str << text;
}

void GrammarAnnotation::Template::PointcutLocation::dump(ostream& str) const
{
  str << "    ::pointcut::\t" << name << endl;
}

void GrammarAnnotation::Template::PointcutLocation::toTXLWithNames(
    ostream&,
    NamingFunction const&) const
{
  SCIS_ERROR("Pointcut locations cannot be represented as TXL");
}

void GrammarAnnotation::Template::TypeReference::dump(ostream& str) const
{
  str << "    ::type::\t" << typeId << endl;
}

void GrammarAnnotation::Template::TypeReference::toTXLWithNames(
    ostream& str,
    NamingFunction const& namer) const
{
  str << namer(typeId) << " [" << typeId << ']';
}

GrammarAnnotation::Template*
GrammarAnnotation::DirectedAcyclicGraph::Keyword::addTemplate(string const& kind)
{
  auto tmpl = make_unique<GrammarAnnotation::Template>();
  tmpl->kind = kind;

  auto const newTemplate = tmpl.get();
  templates.insert_or_assign(newTemplate->kind, std::move(tmpl));

  return newTemplate;
}

GrammarAnnotation::Pointcut*
GrammarAnnotation::DirectedAcyclicGraph::Keyword::addPointcut(string const& name)
{
  auto pcut = make_unique<GrammarAnnotation::Pointcut>();
  pcut->name = name;

  auto const pointcut = pcut.get();
  pointcuts.insert_or_assign(pointcut->name, std::move(pcut));

  return pointcut;
}

GrammarAnnotation::Template const*
GrammarAnnotation::DirectedAcyclicGraph::Keyword::getTemplate(string const& kind) const
{
  if (auto const iTemplate = templates.find(kind); iTemplate != templates.cend())
    return iTemplate->second.get();
  else
    SCIS_ERROR("Missing '" << kind << "' template version "
               "in keyword <" << id << ">");
}

void GrammarAnnotation::Template::toTXLWithNames(
    ostream& str,
    NamingFunction const& namer) const
{
  auto count = blocks.size();
  for (auto const& block : blocks) {
    block->toTXLWithNames(str, namer);

    if (count --> 1)
      str << ' ';
  }
}
