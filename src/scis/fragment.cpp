#include "fragment.h"
#include "logging.h"

using namespace std;
using namespace scis;

void Fragment::TextBlock::dump(ostream &str) const
{
  str << "::text:: " << text;
}

void Fragment::TextBlock::toTXL(ostream& str,
                                unordered_map<string_view, string_view> const&) const
{
  // escape terminal symbols
  bool showQuote = true;
  for (auto const character : text)
    if (character == ' ') {
      showQuote = true;
      str << character;
    }
    else {
      if (showQuote) {
        showQuote = false;
        str << '\'';
      }

      str << character;
    }
  str << ' ';
}

void Fragment::ParamReference::dump(ostream &str) const
{
  str << "::ref:: " << id;
}

void Fragment::ParamReference::toTXL(ostream& str,
                                     unordered_map<string_view, string_view> const& param2arg) const
{
  str << param2arg.at(id) << ' ';
}

void Fragment::dump(ostream &str) const
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

void Fragment::toTXL(ostream& str, vector<string> const& args) const
{
  if (args.size() != params.size())
    SCIS_ERROR("Wrong number of arguments in use of a fragment <" << name << ">");

  // build mapping "parameter <-> argument"
  unordered_map<string_view, string_view> param2arg;
  for (auto arg = args.cbegin(), param = params.cbegin(); arg != args.cend(); arg++, param++)
    param2arg.insert(make_pair(*param, *arg));

  // render
  for (auto const& block : source) {
    block->toTXL(str, param2arg);
    str << endl;
  }
}
