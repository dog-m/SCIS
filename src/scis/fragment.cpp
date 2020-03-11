#include "fragment.h"

using namespace std;
using namespace scis;

void Fragment::TextBlock::dump(ostream &str)
{
  str << "::text:: " << text;
}

void Fragment::ParamReference::dump(ostream &str)
{
  str << "::ref:: " << id;
}

void Fragment::dump(ostream &str)
{
  str << "lang = " << language << endl
      << "name = " << name << endl;

  str << "dependencies: ";
  auto countD = dependencies.size();
  for (auto const& dep : dependencies) {
    str << dep.target
        << (dep.required ? "!" : "");

    if (countD --> 1)
      str << ", ";
  }
  str << endl;

  str << "black list: ";
  auto countBL = black_list.size();
  for (auto const& item : black_list) {
    str << item;

    if (countBL --> 1)
      str << ", ";
  }
  str << endl;

  str << "params: ";
  auto countP = params.size();
  for (auto const& item : params) {
    str << item;

    if (countP --> 1)
      str << ", ";
  }
  str << endl;

  str << "source template:" << endl;
  for (auto const& block : source) {
    block->dump(str);
    str << endl;
  }
}
