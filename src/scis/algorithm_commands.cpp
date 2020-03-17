#include "algorithm_commands.h"

using namespace std;
using namespace scis::algorithm;

using SimpleFunction = function< FunctionCall::Result (FunctionCall const&) >;

static unordered_map<string_view, SimpleFunction> STANDARD_FUNCTIONS {

  { "insert-reference", [](FunctionCall const&) -> FunctionCall::Result
    {
      ;
      return {};
    }
  },

  { "insert-call", [](FunctionCall const&) -> FunctionCall::Result
    {
      ;
      return {};
    }
  },

};

bool call(FunctionCall const& params,
          function<void (FunctionCall::Result const&)> const& resultHandler)
{
  auto const func = STANDARD_FUNCTIONS.find(params.function);
  if (func == STANDARD_FUNCTIONS.cend())
    return false;
  else
    try {
      resultHandler(func->second(params));
      return true;
    } catch (...) {
      return false;
    }
}
