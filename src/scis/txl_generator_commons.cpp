#include "txl_generator_commons.h"

using namespace std;
using namespace scis;

// FIXME: remove duplicates
static string const CURRENT_NODE = "__NODE__";


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
    ss << "  skipping [" << skipType.value() << ']' << endl;

  generateStatements();

  for (auto const& stmt : statements)
    ss << stmt.action << endl
       << stmt.text << endl;

  ss << "end " << ruleOrFunction();
}

string_view TXLFunction::ruleOrFunction()
{
  return isRule ? "rule" : "function";
}

void CallChainFunction::connectTo(CallChainFunction* const other)
{
  this->callTo = other;
  other->callFrom = this;
}

void CollectionFunction::generateStatements()
{
  auto& replaceStmt = statements.emplace_front(/* empty */);
  replaceStmt.action = "replace * [" + processingType + "]";
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
  replaceStmt.action = "replace $ ["s + processingType + ']';
  replaceStmt.text = CURRENT_NODE + " ["s + processingType + "]";

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
  byStmt.text = CURRENT_NODE + " [" + callTo->name + ' ' + paramNamesList + ']';
}

void RefinementFunction::generateStatements()
{
  auto& replaceStmt = statements.emplace_front(/* empty */);
  replaceStmt.action = "replace * [" + processingType + "]";
  replaceStmt.text = CURRENT_NODE + " [" + processingType + "]";

  string paramNamesList = "";
  for (auto const& p : params)
    paramNamesList += p.id + ' ';

  auto& byStmt = statements.emplace_back(/* empty */);
  byStmt.action = "by";
  byStmt.text = CURRENT_NODE + " [" + callTo->name + ' ' + paramNamesList + ']';
}

void InstrumentationFunction::generateStatements()
{
  auto& replaceStmt = statements.emplace_front(/* empty */);
  replaceStmt.action = "replace [" + searchType + "]";
  replaceStmt.text = CURRENT_NODE + " [" + processingType + "]";

  ; // FIXME: InstrumentationFunction::gen is incomplete

  auto& byStmt = statements.emplace_back(/* empty */);
  byStmt.action = "by";
  byStmt.text = replacement;
}
