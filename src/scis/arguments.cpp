#include "arguments.h"
#include "logging.h"

#include <argparse.hpp>

//#include <filesystem> // TODO: use std::filesystem, broken in mingw81
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>

using namespace std;
using namespace scis::args;

constexpr string_view PARAM_SOURCE          = "--src";
constexpr string_view PARAM_SOURCE_SHORTCUT = "-S";

constexpr string_view PARAM_DESTINATION          = "--dst";
constexpr string_view PARAM_DESTINATION_SHORTCUT = "-D";

constexpr string_view PARAM_RULESET          = "--ruleset";
constexpr string_view PARAM_RULESET_SHORTCUT = "-R";

constexpr string_view PARAM_ANNOTATION          = "--annotation";
constexpr string_view PARAM_ANNOTATION_SHORTCUT = "-A";

constexpr string_view PARAM_FRAGMENTS          = "--fragments-dir";
constexpr string_view PARAM_FRAGMENTS_SHORTCUT = "-F";

// TODO: move internal CLI params to settings
constexpr string_view PARAM_INTERNAL_GRM_RULESET  = "--scis-grm-ruleset";
constexpr string_view PARAM_INTERNAL_GRM_TXL      = "--scis-grm-txl";

constexpr string_view PARAM_DISABLED_RULES = "--disable";
constexpr string_view PARAM_NO_CACHE       = "--no-cache";
constexpr string_view PARAM_TXL_ARGS       = "txl_params";


static string normalizePath(string const& path)
{
  return boost::filesystem::absolute(path).lexically_normal().make_preferred().string();
}


static string getFilenameParameter(argparse::ArgumentParser& parser, string_view const& name)
{
  auto const path = parser.get<string>(name);
  return normalizePath(path);
}


void scis::args::updateArguments(int const argc,
                                 char** const argv)
{
  argparse::ArgumentParser parser("scis");

  // describe all possible options
  parser
      .add_argument(PARAM_INTERNAL_GRM_TXL)
      .help("standard grammar for TXL language\t")
      .required();


  parser
      .add_argument(PARAM_INTERNAL_GRM_RULESET)
      .help("standard grammar for a set of rules (ruleset)\t")
      .required();


  parser
      .add_argument(PARAM_SOURCE, PARAM_SOURCE_SHORTCUT)
      .help("program source text file to be processed\t")
      .required();

  parser
      .add_argument(PARAM_DESTINATION, PARAM_DESTINATION_SHORTCUT)
      .help("processed program text will be saved as\t")
      .required();

  parser
      .add_argument(PARAM_RULESET, PARAM_RULESET_SHORTCUT)
      .help("ruleset\t")
      .required();

  parser
      .add_argument(PARAM_ANNOTATION, PARAM_ANNOTATION_SHORTCUT)
      .help("annotation\t")
      .required();

  parser
      .add_argument(PARAM_FRAGMENTS, PARAM_FRAGMENTS_SHORTCUT)
      .help("fragments dir\t")
      .required();

  parser
      .add_argument(PARAM_DISABLED_RULES)
      .help("-")
      .default_value(string());

  parser
      .add_argument(PARAM_NO_CACHE)
      .help("-")
      .default_value(false)
      .implicit_value(true);

  parser
      .add_argument(PARAM_TXL_ARGS)
      .remaining()
      .default_value(vector<string>());

  // perform parsing
  try {
    parser.parse_args(argc, argv);
  }
  catch (runtime_error const& err) {
    SCIS_ERROR(err.what() << endl << parser);
  }

  // internal values
  ARG_EXECUTABLE_DIR        = normalizePath(boost::filesystem::path(argv[0]).parent_path().string());
  ARG_WORKING_DIR           = boost::filesystem::current_path().string();
  ARG_INTERNAL_GRM_TXL      = getFilenameParameter(parser, PARAM_INTERNAL_GRM_TXL);
  ARG_INTERNAL_GRM_RULESET  = getFilenameParameter(parser, PARAM_INTERNAL_GRM_RULESET);

  // extract parsed data from parser
  ARG_SRC_FILENAME  = getFilenameParameter(parser, PARAM_SOURCE);
  ARG_DST_FILENAME  = getFilenameParameter(parser, PARAM_DESTINATION);
  ARG_RULESET       = getFilenameParameter(parser, PARAM_RULESET);
  ARG_ANNOTATION    = getFilenameParameter(parser, PARAM_ANNOTATION);
  ARG_FRAGMENTS_DIR = getFilenameParameter(parser, PARAM_FRAGMENTS);

  ARG_DISABLED_RULES  = parser.get<string>(PARAM_DISABLED_RULES);
  ARG_USE_CACHE       = !parser.get<bool>(PARAM_NO_CACHE);

  ARG_TXL_PARAMETERS = "";
  for (auto const& param : parser.get<vector<string>>(PARAM_TXL_ARGS))
    ARG_TXL_PARAMETERS += ' ' + param;

  // post-processing
  ARG_ANNOTATION_DIR = normalizePath(boost::filesystem::path(ARG_ANNOTATION).parent_path().string());
}

void scis::args::updateGrammarLocation(string const& grmRelativePath)
{
  ARG_GRAMMAR = normalizePath(ARG_ANNOTATION_DIR + '/' + grmRelativePath);
}
