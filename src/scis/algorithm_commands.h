#ifndef ALGORITHM_COMMANDS_H
#define ALGORITHM_COMMANDS_H

#include <string>
#include <unordered_map>
#include <functional>

namespace scis::algorithm {

  using namespace std;

  /// pass data around only
  struct FunctionCall {
    struct Result {
      //string replaceText;
      string byText;
    };

    string const& function;
    unordered_map<string, string> const& args;
    string const& preparedFragment;
  }; // FunctionCall

  bool call(FunctionCall const& params,
            function<void(FunctionCall::Result const&)> const& resultHandler);

} // scis

#endif // ALGORITHM_COMMANDS_H
