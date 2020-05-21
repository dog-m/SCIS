#include "interpreter.h"
#include "wrapper.h"
#include "../scis/cli_arguments.h"

#include <sstream>

using namespace std;

void txl::Interpreter::test()
{
  txl::Wrapper::runNoInput({ txl::PARAM_HELP },
                           txl::Wrapper::READER_LOG,
                           txl::Wrapper::READER_LOG);
}

string txl::Interpreter::grammarToXML(string_view const& filename)
{
  stringstream result;

  txl::Wrapper::runNoInput({ filename.data(), scis::args::ARG_INTERNAL_GRM_TXL, txl::PARAM_XML_RESULT },
                           txl::Wrapper::READER_NOOP,
                           [&](string const& line) {
    result << line << endl;
    return true;
  });

  return result.str();
}

string txl::Interpreter::rulesetToXML(string_view const& filename)
{
  stringstream result;

  // FIXME: add error handling
  txl::Wrapper::runNoInput({ filename.data(), scis::args::ARG_INTERNAL_GRM_RULESET, txl::PARAM_XML_RESULT },
                           txl::Wrapper::READER_NOOP,
                           [&](string const& line) {
    result << line << endl;
    return true;
  });

  return result.str();
}
