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

  auto const err = txl::Wrapper::runNoInput({ filename.data(), scis::args::ARG_INTERNAL_GRM_TXL, txl::PARAM_XML_RESULT },
                           txl::Wrapper::READER_NOOP,
                           [&](string const& line) {
    result << line << endl;
    return true;
  });

  // TODO: use logging or exceptions for parsing
  if (err != 0) {
    // print message
    txl::Wrapper::READER_LOG("Ruleset parsing error. TXL output:");
    // print output
    txl::Wrapper::READER_LOG(result.str());
    result.str("");
  }

  return result.str();
}

string txl::Interpreter::rulesetToXML(string_view const& filename)
{
  stringstream result;

  auto const err = txl::Wrapper::runNoInput({ filename.data(), scis::args::ARG_INTERNAL_GRM_RULESET, txl::PARAM_XML_RESULT },
                           txl::Wrapper::READER_NOOP,
                           [&](string const& line) {
    result << line << endl;
    return true;
  });

  // TODO: use logging or exceptions for parsing
  if (err != 0) {
    // print message
    txl::Wrapper::READER_LOG("Ruleset parsing error. TXL output:");
    // print output
    txl::Wrapper::READER_LOG(result.str());
    result.str("");
  }

  return result.str();
}
