#include "txl_generator_commons.h"

using namespace std;
using namespace scis::codegen;

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

TXLFunction::Statement& TXLFunction::addStatementTop(const string& action, const string& text)
{
  auto& stmt = statements.emplace_front(/* empty */);
  stmt.action = action;
  stmt.text = text;
  return stmt;
}

TXLFunction::Statement& TXLFunction::addStatementBott(const string& action, const string& text)
{
  auto& stmt = statements.emplace_back(/* empty */);
  stmt.action = action;
  stmt.text = text;
  return stmt;
}

TXLFunction::Statement& TXLFunction::createVariable(
    string const& name,
    string const& type,
    string const& value)
{
  return addStatementBott(
        "construct " + name + " [" + type + "]",
        value);
}

TXLFunction::Parameter& TXLFunction::addParameter(
    string const& name,
    string const& type)
{
  auto& param = params.emplace_back(/* empty */);
  param.id = name;
  param.type = type;
  return param;
}

TXLFunction::Statement& TXLFunction::deconstructVariable(
    string const& name,
    string const& pattern)
{
  return addStatementBott(
        "deconstruct " + name,
        pattern);
}

void CallChainFunction::connectTo(CallChainFunction* const other)
{
  this->callTo = other;
  other->callFrom = this;
}

void CollectionFunction::generateStatements()
{
  addStatementTop(
        "replace $ [" + processingType + "]",
        CURRENT_NODE + " [" + processingType + "]");

  string paramNamesList = "";
  for (auto const& p : params)
    paramNamesList += ' ' + p.id;

  addStatementBott(
        "by",
        CURRENT_NODE + " [" + callTo->name + paramNamesList + CURRENT_NODE + ']');
}

void FilteringFunction::generateStatements()
{
  addStatementTop(
        "replace $ [" + processingType + "]",
        CURRENT_NODE + " [" + processingType + "]");

  for (auto const& where : wheres) {
    auto constraint = where.target;

    for (auto const& op : where.operators) {
      constraint += " [" + op.name;

      for (auto const& arg : op.args)
        constraint += ' ' + arg;

      constraint += "]";
    }

    addStatementBott("where", constraint);
  }

  string paramNamesList = "";
  for (auto const& p : params)
    paramNamesList += ' ' + p.id;

  addStatementBott(
        "by",
        CURRENT_NODE + " [" + callTo->name + paramNamesList + "]");
}

void RefinementFunction::generateStatements()
{
  string const repeatModifier = isRule ? "$" : "*";

  addStatementTop(
        "replace " + repeatModifier + " [" + processingType + "]",
        CURRENT_NODE + " [" + processingType + "]");

  string paramNamesList = "";
  for (auto const& p : params)
    paramNamesList += ' ' + p.id;

  addStatementBott(
        "by",
        CURRENT_NODE + " [" + callTo->name + paramNamesList + "]");
}

void InstrumentationFunction::generateStatements()
{
  addStatementTop(
        "replace [" + searchType + "]",
        CURRENT_NODE + " [" + processingType + "]");

  addStatementBott(
        "by",
        replacement);
}

string scis::codegen::makeFunctionNameFromContextName(string const& context,
                                                      bool const negative)
{
  return (negative ? CONTEXT_FUNCTION_NEGATIVE_PREFIX : CONTEXT_FUNCTION_PREFIX)
         + context;
}

string scis::codegen::getUniqueId()
{
  static uint64_t id = 0;
  return "uid" + to_string(id++);
}

string scis::codegen::makeNameFromType(string_view const& typeName)
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

string scis::codegen::makeNameFromKeyword(string_view const& keywordName)
{
  return "kw_" + makeNameFromType(keywordName);
}

string scis::codegen::makeNameFromPOIName(string const& poi)
{
  string processedPOI = poi;

  for (auto& c : processedPOI)
    c = isalpha(c) ? c : '_';

  return processedPOI;
}

string scis::codegen::makeFunctionNameFromPOIName(string const& poi)
{
  return POI_GETTER_PREFIX + makeNameFromPOIName(poi);
}
