#include "algorithm_commands.h"
#include <sstream>

using namespace std;
using namespace scis;

using SimpleFunction = function< FunctionCall::Result (FunctionCall const&) >;

static unordered_map<string_view, SimpleFunction> STANDARD_FUNCTIONS {

  { "insert-fragment", [](FunctionCall const& call) -> FunctionCall::Result
    {
      auto const left = call.args.find("each-line-prefix");
      auto const prefixWithSpace = left != call.args.cend() ? left->second + ' ' : "";

      auto const right = call.args.find("each-line-postfix");
      auto const postfixWithSpace = right != call.args.cend() ? ' ' + right->second : "";

      string processedFragment = "";

      stringstream ss(call.preparedFragment);
      string line;
      while (getline(ss, line))
        processedFragment += prefixWithSpace + line + postfixWithSpace + '\n';

      return { .byText = processedFragment };
    }
  },

  { "insert-call", [](FunctionCall const& call) -> FunctionCall::Result
    {
      auto const& func = call.args.at("function");
      auto const& params = call.args.at("params");

      return { .byText = "[" + func + " " + params + "]" };
    }
  },

  { "insert-text", [](FunctionCall const& call) -> FunctionCall::Result
    {
      return { .byText = call.args.at("text") };
    }
  },

  { "fragment-to-variable", [](FunctionCall const& call) -> FunctionCall::Result
    {
      auto const& varName = call.args.at("name");
      auto const& varType = call.args.at("type");

      auto const left = call.args.find("each-line-prefix");
      auto const prefixWithSpace = left != call.args.cend() ? left->second + ' ' : "";

      auto const right = call.args.find("each-line-postfix");
      auto const postfixWithSpace = right != call.args.cend() ? ' ' + right->second : "";

      string processedFragment = "";

      stringstream ss(call.preparedFragment);
      string line;
      while (getline(ss, line))
        processedFragment += prefixWithSpace + line + postfixWithSpace + '\n';
      // remove last "new line" symbol
      processedFragment.pop_back();

      call.iFunc->createVariable(varName, varType, processedFragment);

      return {};
    }
  },

  { "deconstruct-variable", [](FunctionCall const& call) -> FunctionCall::Result
    {
      auto const& type = call.args.at("type");
      // BUG: check boundaries
      auto const index = std::stoi(call.args.at("variant"));
      auto const variant = index < 0 ? 0 : index;

      stringstream pattern;
      call.grammar->types.at(type)->variants[variant].toTXLWithNames(pattern, codegen::makeNameFromType);

      call.iFunc->deconstructVariable(codegen::makeNameFromType(type), pattern.str());

      return {};
    }
  },

  { "create-variable", [](FunctionCall const& call) -> FunctionCall::Result
    {
      auto const& type = call.args.at("type");
      auto const& value = call.args.at("value");

      auto const proposedName = call.args.find("name");
      auto const name = proposedName != call.args.cend() ? proposedName->second : codegen::makeNameFromType(type);

      call.iFunc->createVariable(name, type, value);

      return {};
    }
  }

};

bool scis::callAlgorithmCommand(FunctionCall&& params,
                                FunctionResultHandler&& resultHandler)
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
