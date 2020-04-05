#include "cli.h"
#include "logging.h"
#include <argparse.hpp>

using namespace std;
using namespace scis;

constexpr string_view PARAM_SOURCE          = "--src";
constexpr string_view PARAM_SOURCE_SHORTCUT = "-s";

constexpr string_view PARAM_DESTINATION          = "--dst";
constexpr string_view PARAM_DESTINATION_SHORTCUT = "-d";

constexpr string_view PARAM_RULESET          = "--ruleset";
constexpr string_view PARAM_RULESET_SHORTCUT = "-r";

constexpr string_view PARAM_GRAMMAR          = "--grammar";
constexpr string_view PARAM_GRAMMAR_SHORTCUT = "-g";

constexpr string_view PARAM_ANNOTATION          = "--annotation";
constexpr string_view PARAM_ANNOTATION_SHORTCUT = "-a";

constexpr string_view PARAM_FRAGMENTS          = "--fragments-dir";
constexpr string_view PARAM_FRAGMENTS_SHORTCUT = "-f";

constexpr string_view PARAM_DISABLED_RULES = "--disable";
constexpr string_view PARAM_NO_CACHE       = "--no-cache";
constexpr string_view PARAM_TXL_ARGS       = "txl";


void CLI::parseArguments(int const argc,
                         char** const argv)
{
  argparse::ArgumentParser parser("Source Code Instrumentation System");

  // describe all possible options
  parser
      .add_argument(PARAM_SOURCE, PARAM_SOURCE_SHORTCUT)
      .help("Program source text file to be processed")
      .required();

  parser
      .add_argument(PARAM_DESTINATION, PARAM_DESTINATION_SHORTCUT)
      .help("Processed program text will be saved as")
      .required();

  parser
      .add_argument(PARAM_RULESET, PARAM_RULESET_SHORTCUT)
      .help("ruleset")
      .required();

  parser
      .add_argument(PARAM_GRAMMAR, PARAM_GRAMMAR_SHORTCUT)
      .help("grammar")
      .required();

  parser
      .add_argument(PARAM_ANNOTATION, PARAM_ANNOTATION_SHORTCUT)
      .help("annotation")
      .required();

  parser
      .add_argument(PARAM_FRAGMENTS, PARAM_FRAGMENTS_SHORTCUT)
      .help("fragments dir")
      .required();

  parser
      .add_argument(PARAM_DISABLED_RULES)
      .help("-")
      .default_value("");

  parser
      .add_argument(PARAM_NO_CACHE)
      .help("-")
      .default_value(false);

  parser
      .add_argument(PARAM_TXL_ARGS)
      .remaining();

  // perform parsing
  try {
    parser.parse_args(argc, argv);
  }
  catch (runtime_error const& err) {
    SCIS_ERROR(err.what());
  }

  // extract parsed data from parser
  srcFile = parser.get<string>(PARAM_SOURCE);
  dstFile = parser.get<string>(PARAM_DESTINATION);
  ruleset = parser.get<string>(PARAM_RULESET);
  grammar = parser.get<string>(PARAM_GRAMMAR);
  annotation = parser.get<string>(PARAM_ANNOTATION);
  fragmentsDir = parser.get<string>(PARAM_FRAGMENTS);

  disabledRules = parser.get<string>(PARAM_DISABLED_RULES);

  cacheEnabled = !parser.get<bool>(PARAM_NO_CACHE);

  txlParams = "";
  for (auto const& param : parser.get<vector<string>>(PARAM_TXL_ARGS))
    txlParams += ' ' + param;
}
