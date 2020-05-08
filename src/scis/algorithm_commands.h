#ifndef ALGORITHM_COMMANDS_H
#define ALGORITHM_COMMANDS_H

#include "txl_generator_commons.h"
#include "../txl/grammar.h"
#include <functional>

namespace scis {

  using namespace std;

  /// pass data around only
  struct FunctionCall final {
    struct Result final {
      //string replaceText;
      string byText;
    };

    string function;
    unordered_map<string, string> args;
    string preparedFragment;
    codegen::InstrumentationFunction* iFunc;
    txl::Grammar const* grammar;
  }; // FunctionCall

  using FunctionResultHandler = function<void (FunctionCall::Result const&)>;

  bool callAlgorithmCommand(FunctionCall&& params,
                            FunctionResultHandler&& resultHandler);

} // scis

#endif // ALGORITHM_COMMANDS_H
