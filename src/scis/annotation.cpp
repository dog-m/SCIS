#include "annotation.h"

using namespace std;
using namespace scis;

void GrammarAnnotation::dump(ostream& str)
{
  str << "pipeline: " << pipeline << endl  // TODO: unused pipeline string in annotation
      << "Grammar:" << endl
      << "  lang = " << grammar.language << endl
      << "  src = " << grammar.txlSourceFilename << endl
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

    auto countP = keyword->replacement_patterns.size();
    str << "  patterns (" << countP << "):" << endl;
    for (auto const& pattern : keyword->replacement_patterns) {
      str << "    search type = [" << pattern.searchType << "]" << endl;
      for (auto const& block : pattern.blocks)
        block->dump(str);

      if (countP --> 1)
        str << "  ....." << endl;
    }
    
    str << "  pointcuts (" << keyword->pointcuts.size() << "):" << endl;
    for (auto const& [_, point] : keyword->pointcuts) {
      str << "    name: \'" << point->name << '\'' << endl
          << "    ref: " << point->fragAlias << " [" << point->fragType << "]" << endl;

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

void GrammarAnnotation::Pattern::TextBlock::dump(ostream& str)
{
  str << "    ::text::\t" << text << endl;
}

void GrammarAnnotation::Pattern::PointcutLocation::dump(ostream& str)
{
  str << "    ::pointcut::\t" << name << endl;
}

void GrammarAnnotation::Pattern::TypeReference::dump(ostream& str)
{
  str << "    ::type::\t" << typeId << endl;
}
