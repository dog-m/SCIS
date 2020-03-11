#include "interpreter.h"
#include "wrapper.h"

#include <sstream>

using namespace std;

void txl::Interpreter::test()
{
  txl::Wrapper::runNoInput({ "-v" },
                           [](string const& line) {
    cout << line << endl;
    return true;
  });
}

string txl::Interpreter::grammarToXML(string_view const& fileName)
{
  stringstream result;

  txl::Wrapper::runNoInput({ fileName.data(), "./txl_grammar.txl", "-xml" },
                           txl::Wrapper::NOOP_READER,
                           [&](string const& line) {
    result << line << endl;
    return true;
  });

  return result.str();
}

string txl::Interpreter::rulesetToXML(string_view const& fileName)
{
  stringstream result;

  txl::Wrapper::runNoInput({ fileName.data(), "./ruleset_grammar.txl", "-xml" },
                           txl::Wrapper::NOOP_READER, // FIXME: add error handling
                           [&](string const& line) {
    result << line << endl;
    return true;
  });

  return result.str();
}
