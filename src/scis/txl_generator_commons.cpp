#include "txl_generator_commons.h"

using namespace std;
using namespace scis::codegen;

void TXLFunction::copyParamsFrom(TXLFunction const* const from)
{
  if (!from)
    return;

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

TXLFunction::Statement& TXLFunction::importVariable(const string& name, const string& type)
{
  return addStatementBott(
        "import " + name + " [" + type + "]");
}

TXLFunction::Statement& TXLFunction::exportVariableCreate(
    string const& name,
    string const& type,
    string const& newValue)
{
  return addStatementBott(
        "export " + name + " [" + type + "]",
        newValue);
}

TXLFunction::Statement& TXLFunction::exportVariableUpdate(
    string const& name,
    string const& newValue)
{
  return addStatementBott(
        "export " + name,
        newValue);
}

string TXLFunction::getRepeatModifier()
{
  return isRule ? "$" : "*";
}

string TXLFunction::getParamNames()
{
  string paramNamesList {""};
  for (auto const& param : params)
    paramNamesList += ' ' + param.id;

  return paramNamesList;
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
        NODE_CURRENT + " [" + processingType + "]");

  addStatementBott(
        "by",
        NODE_CURRENT + " [" + callTo->name + getParamNames() + " " + NODE_CURRENT + ']');
}

void FilteringFunction::generateStatements()
{
  addStatementTop(
        "replace $ [" + processingType + "]",
        NODE_CURRENT + " [" + processingType + "]");

  addStatementBott(
        "by",
        NODE_CURRENT + " [" + callTo->name + getParamNames() + "]");
}

void RefinementFunctionFirst::generateStatements()
{
  if (sequential) {
    addStatementTop(
          "replace " + getRepeatModifier() + " [" + searchType + "]",
          NODE_CURRENT + " [" + processingType + "] " + NODE_SEQ_TAIL + " [" + searchType + "]");

    createVariable(
          NODE_SEQ_SINGLE, searchType,
          NODE_CURRENT + " % +empty");

    createVariable(
          NODE_SEQ_PROCESSED, searchType,
          NODE_SEQ_SINGLE + " [" + callTo->name + getParamNames() + "]");

    addStatementBott(
          "by",
          NODE_SEQ_PROCESSED + " [. " + NODE_SEQ_TAIL + "]");
  }
  else {
    addStatementTop(
          "replace " + getRepeatModifier() + " [" + searchType + "]",
          NODE_CURRENT + " [" + processingType + "]");

    addStatementBott(
          "by",
          NODE_CURRENT + " [" + callTo->name + getParamNames() + "]");
  }
}

void RefinementFunctionAll::generateStatements()
{
  if (sequential) {
    addStatementTop(
          "replace " + getRepeatModifier() + " [" + searchType + "]",
          NODE_CURRENT + " [" + processingType + "] " + NODE_SEQ_TAIL + " [" + searchType + "]");

    createVariable(
          NODE_SEQ_SINGLE, searchType,
          NODE_CURRENT + " % +empty");

    createVariable(
          NODE_SEQ_PROCESSED, searchType,
          NODE_SEQ_SINGLE + " [" + callTo->name + getParamNames() + "]");

    addStatementBott(
          "by",
          NODE_SEQ_PROCESSED + " [. " + NODE_SEQ_TAIL + "]");
  }
  else {
    addStatementTop(
          "replace " + getRepeatModifier() + " [" + searchType + "]",
          NODE_CURRENT + " [" + processingType + "]");

    addStatementBott(
          "by",
          NODE_CURRENT + " [" + callTo->name + getParamNames() + "]");
  }
}

void InstrumentationFunction::generateStatements()
{
  addStatementTop(
        "replace [" + searchType + "]",
        NODE_CURRENT + " [" + processingType + "]");

  addStatementBott(
        "by",
        replacement);
}

string scis::codegen::makeFunctionNameFromContextName(string const& context,
                                                      bool const negative)
{
  return (negative ? PREFIX_CONTEXT_FUNCTION_NEG : PREFIX_CONTEXT_FUNCTION)
         + context;
}

string scis::codegen::getUniqueId()
{
  static uint64_t id = 0;
  return PREFIX_UNIQUE_ID + to_string(id++);
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
  return PREFIX_VAR_KEYWORD + makeNameFromType(keywordName);
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
  return PREFIX_POI_GETTER + makeNameFromPOIName(poi);
}
