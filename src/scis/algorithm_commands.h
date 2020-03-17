#ifndef ALGORITHM_COMMANDS_H
#define ALGORITHM_COMMANDS_H

#include <string>
#include <unordered_map>
#include <functional>
#include "txl_generator_commons.h"

namespace scis {

  using namespace std;
  //using namespace scis::generation;

  /// pass data around only
  struct FunctionCall {
    struct Result {
      //string replaceText;
      string byText;
    };

    string function;
    unordered_map<string, string> args;
    string preparedFragment;
    InstrumentationFunction * iFunc;
  }; // FunctionCall

  using FFF = function<void (FunctionCall::Result const&)>;

  bool call(FunctionCall const& params,
            FFF const& resultHandler);

  string getUniqueId();

  string typeToName(string_view const& typeName);

} // scis

#endif // ALGORITHM_COMMANDS_H
