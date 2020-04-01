#include "algorithm_commands.h"
#include <sstream>

using namespace std;
using namespace scis;

using SimpleFunction = function< FunctionCall::Result (FunctionCall const&) >;

static unordered_map<string_view, SimpleFunction> STANDARD_FUNCTIONS {

  { "insert-fragment", [](FunctionCall const& call) -> FunctionCall::Result
    {
      return { .byText = call.preparedFragment };
    }
  },

  { "insert-call", [](FunctionCall const& call) -> FunctionCall::Result
    {
      return { .byText = "[" + call.args.at("function") + " " + call.args.at("params") + "]" };
    }
  },

  { "insert-text", [](FunctionCall const& call) -> FunctionCall::Result
    {
      string text = call.args.at("text") + " ";
      return { .byText = text };
    }
  },

  { "fragment-to-variable", [](FunctionCall const& call) -> FunctionCall::Result
    {
      call.iFunc->createVariable(call.args.at("name"), call.args.at("type"), call.preparedFragment);

      return {};
    }
  },

  { "deconstruct-variable", [](FunctionCall const& call) -> FunctionCall::Result
    {
      auto const& type = call.args.at("type");
      // BUG: check boundaries
      uint8_t const variant = atoi(call.args.at("variant").data());

      stringstream pattern;
      call.grammar->types.at(type)->variants[variant].toTXLWithNames(pattern, codegen::makeNameFromType);

      call.iFunc->deconstructVariable(codegen::makeNameFromType(type), pattern.str());

      return {};
    }
  },

  { "create-variable", [](FunctionCall const& call) -> FunctionCall::Result
    {
      call.iFunc->createVariable(call.args.at("name"), call.args.at("type"), call.args.at("value"));

      return {};
    }
  }

};

bool scis::callAlgorithmCommand(FunctionCall const& params,
                                FunctionResultHandler const& resultHandler)
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
