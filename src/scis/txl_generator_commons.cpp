#include "txl_generator_commons.h"

using namespace std;
using namespace scis;

void TXLFunction::copyParamsFrom(TXLFunction const* const from)
{
  params.insert(params.end(), from->params.cbegin(), from->params.cend());
}

void TXLFunction::generateTXL(ostream& ss)
{
  ss << ruleOrFunction() << " " << name;
  for (auto const& p : params)
    ss << ' ' << p.id << " [" << p.type << ']';
  ss << endl;

  if (skipType.has_value())
    ss << '\t' << "skipping [" << skipType.value() << ']' << endl;

  generateStatements();

  for (auto const& stmt : statements)
    ss << '\t' << stmt.action << endl
       << "\t\t" << stmt.text << endl;

  ss << "end " << ruleOrFunction() << endl;
}

string_view TXLFunction::ruleOrFunction()
{
  return isRule ? "rule" : "function";
}

void TXLFunction::createVariable(string const& name,
                                 string const& type,
                                 string const& value)
{
  auto& stmt = statements.emplace_back(/* empty */);
  stmt.action = "construct " + name + " [" + type + "]";
  stmt.text = value;
}

void TXLFunction::addParameter(string const& name,
                               string const& type)
{
  auto& param = params.emplace_back(/* empty */);
  param.id = name;
  param.type = type;
}

void TXLFunction::deconstructVariable(string const& name,
                                      string const& pattern)
{
  auto& stmt = statements.emplace_back(/* empty */);
  stmt.action = "deconstruct " + name;
  stmt.text = pattern;
}

void CallChainFunction::connectTo(CallChainFunction* const other)
{
  this->callTo = other;
  other->callFrom = this;
}

void CollectionFunction::generateStatements()
{
  auto& replaceStmt = statements.emplace_front(/* empty */);
  replaceStmt.action = "replace $ [" + processingType + "]";
  replaceStmt.text = CURRENT_NODE + " [" + processingType + "]";

  string paramNamesList = "";
  for (auto const& p : params)
    paramNamesList += p.id + ' ';

  auto& byStmt = statements.emplace_back(/* empty */);
  byStmt.action = "by";
  byStmt.text = CURRENT_NODE + " [" + callTo->name + ' ' + paramNamesList + CURRENT_NODE + ']';
}

void FilteringFunction::generateStatements()
{
  auto& replaceStmt = statements.emplace_front(/* empty */);
  replaceStmt.action = "replace $ [" + processingType + ']';
  replaceStmt.text = CURRENT_NODE + " [" + processingType + "]";

  for (auto const& where : wheres) {
    auto& whereStmt = statements.emplace_back(/* empty */);
    whereStmt.action = "where";

    whereStmt.text = where.target;
    for (auto const& op : where.operators) {
      whereStmt.text += " [" + op.name;

      for (auto const& arg : op.args)
        whereStmt.text += ' ' + arg;

      whereStmt.text += "]";
    }
  }

  string paramNamesList = "";
  for (auto const& p : params)
    paramNamesList += p.id + ' ';

  auto& byStmt = statements.emplace_back(/* empty */);
  byStmt.action = "by";
  byStmt.text = CURRENT_NODE + " [" + callTo->name + " " + paramNamesList + "]";
}

void RefinementFunction::generateStatements()
{
  string const repeatModifier = isRule ? "$" : "*";

  auto& replaceStmt = statements.emplace_front(/* empty */);
  replaceStmt.action = "replace " + repeatModifier + " [" + processingType + "]";
  replaceStmt.text = CURRENT_NODE + " [" + processingType + "]";

  string paramNamesList = "";
  for (auto const& p : params)
    paramNamesList += p.id + ' ';

  auto& byStmt = statements.emplace_back(/* empty */);
  byStmt.action = "by";
  byStmt.text = CURRENT_NODE + " [" + callTo->name + " " + paramNamesList + "]";
}

void InstrumentationFunction::generateStatements()
{
  auto& replaceStmt = statements.emplace_front(/* empty */);
  replaceStmt.action = "replace [" + searchType + "]";
  replaceStmt.text = CURRENT_NODE + " [" + processingType + "]";

  auto& byStmt = statements.emplace_back(/* empty */);
  byStmt.action = "by";
  byStmt.text = replacement;
}

string scis::contextNameToFunctionName(string const& context,
                                       bool const negative)
{
  return negative ? CONTEXT_FUNCTION_NEGATIVE_PREFIX : CONTEXT_FUNCTION_PREFIX
         + context;
}
