#ifndef ALGORITHM_COMMANDS_H
#define ALGORITHM_COMMANDS_H

#include "txl_generator_commons.h"
#include <functional>

namespace scis {

  /// pass data around only
  struct FunctionCall {
    struct Result {
      //string replaceText;
      std::string byText;
    };

    std::string function;
    std::unordered_map<std::string, std::string> args;
    std::string preparedFragment;
    /*generation::*/InstrumentationFunction * iFunc;
  }; // FunctionCall

  using FunctionResultHandler = std::function<void (FunctionCall::Result const&)>;

} // scis

bool callAlgorithmCommand(scis::FunctionCall const& params,
                          scis::FunctionResultHandler const& resultHandler);

std::string getUniqueId();

std::string typeToName(std::string_view const& typeName);

#endif // ALGORITHM_COMMANDS_H
