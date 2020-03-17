#include "algorithm_commands.h"

using namespace std;
using namespace scis;

using SimpleFunction = function< FunctionCall::Result (FunctionCall const&) >;

static unordered_map<string_view, SimpleFunction> STANDARD_FUNCTIONS {

  { "insert-fragment", [](FunctionCall const& call) -> FunctionCall::Result
    {
      return { .byText = call.preparedFragment };
    }
  },

  { "insert-call", [](FunctionCall const& call) -> FunctionCall::Result // FIXME: !!! incomplete !!!
    {
      return { .byText = "[" + call.args.at("function") + " " + call.args.at("params") + "]" };
    }
  },

  { "insert-text", [](FunctionCall const& call) -> FunctionCall::Result // FIXME: !!! incomplete !!!
    {
      string text = call.args.at("text") + " ";
      return { .byText = text };
    }
  },

  { "fragment-to-variable", [](FunctionCall const& call) -> FunctionCall::Result // FIXME: !!! incomplete !!!
    {
      auto& stmt = call.iFunc->statements.emplace_back();
      stmt.action = "construct " + call.args.at("name") + " [" + call.args.at("type") + "]";
      stmt.text = call.preparedFragment;

      return {};
    }
  },

};

bool callAlgorithmCommand(FunctionCall const& params,
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

string getUniqueId()
{
  static uint64_t id = 0;
  return "uid" + to_string(id++);
}

string typeToName(const string_view& typeName)
{
  string processed;

  bool useUpperCase = true;
  for (auto const c : typeName) {
    if (c == ' ' || c == '_') {
      useUpperCase = true;
      continue;
    }

    if (useUpperCase) {
      processed.push_back(toupper(c));
      useUpperCase = false;
    }
    else
      processed.push_back(c);
  }

  return processed;
}
