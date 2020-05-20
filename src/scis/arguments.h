#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <string>

namespace scis::args {

  using namespace std;

  // TODO: move internal CLI params to settings
  inline string ARG_WORKING_DIR          = "";
  inline string ARG_EXECUTABLE_DIR       = "";
  inline string ARG_INTERNAL_GRM_TXL     = "";
  inline string ARG_INTERNAL_GRM_RULESET = "";

  inline string ARG_ANNOTATION_DIR = "";
  inline string ARG_SRC_FILENAME   = "";
  inline string ARG_DST_FILENAME   = "";
  inline string ARG_RULESET        = "";
  inline string ARG_GRAMMAR        = "";
  inline string ARG_ANNOTATION     = "";
  inline string ARG_FRAGMENTS_DIR  = "";
  inline string ARG_DISABLED_RULES = ""; // TODO: set of strings
  inline bool   ARG_USE_CACHE      = true;
  inline string ARG_TXL_PARAMETERS = "";

  void updateArguments(int const argc,
                       char** const argv);

  void updateGrammarLocation(string const& grmRelativePath);

};

#endif // ARGUMENTS_H
