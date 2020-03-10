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

string txl::Interpreter::grammarToXML(string_view const& grammarFileName)
{
  stringstream result;

  txl::Wrapper::runNoInput({ grammarFileName.data(), "./txl_grammar.txl", "-xml" },
                           txl::Wrapper::NOOP_READER,
                           [&](string const& line) {
    result << line << endl;
    return true;
  });

  return result.str();
}
