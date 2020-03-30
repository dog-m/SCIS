#include "txl_generator.h"
#include "logging.h"

#include <functional>
#include <deque>
#include <unordered_set>
#include <sstream>

#include "algorithm_commands.h"

using namespace std;
using namespace scis;

constexpr auto DAG_DISTANCES_SEARCH_LIMMIT = 2500;

constexpr auto CONTEXT_DEPENDENCY_WAITING_LIMMIT = 5200;

static unordered_map<string_view, string_view> OPERATOR_INVERSION_MAPPING {
  { "=", "~=" },
  { "~=", "=" },
  { "<", ">=" },
  { "<=", ">" },
  { ">", "<=" },
  { ">=", "<" },
};

static unordered_map<string, pair<string, string>> OPERATOR_WRAPPER_MAPPING {
  // TODO: make better approach
  { "=",        { "=",    PREFIX_STD + "equal"         }},
  { "~=",       { "~=",   PREFIX_STD + "not_equal"     }},
  { "<",        { "<",    PREFIX_STD + "lower"         }},
  { "<=",       { "<=",   PREFIX_STD + "lower_equal"   }},
  { ">",        { ">",    PREFIX_STD + "greater"       }},
  { ">=",       { ">=",   PREFIX_STD + "greater_equal" }},
  { "contains", { "grep", PREFIX_STD + "contains"      }},
};


Fragment const* TXLGenerator::getFragment(string_view const& id)
{
  auto const frag = fragments.find(id);
  if (frag != fragments.cend())
    return frag->second.get();

  else
    SCIS_ERROR("Unknown fragment id <" << id << ">");
}

void TXLGenerator::addToCallChain(CallChainFunction *const func)
{
  if (!currentCallChain.empty())
    currentCallChain.back()->connectTo(func);

  currentCallChain.push_back(func);
}

void TXLGenerator::resetCallChain()
{
  currentCallChain.clear();
}

TXLFunction* TXLGenerator::lastCallChainElement()
{
  return currentCallChain.empty() ? nullptr : currentCallChain.back();
}

void TXLGenerator::wrapStandardBinnaryOperator(
    string const& op,
    string const& type,
    string const& name,
    string const& internalFunction)
{
  // create simple function and fill basic info
  auto const wrapper = createFunction<TXLFunction>();
  wrapper->isRule = false;
  wrapper->name = name;

  // add params
  wrapper->addParameter("A", type);
  wrapper->addParameter("B", type);

  // always match
  wrapper->addStatementTop(
        "match [any]",
        "_ [any]");

  // call original operator
  wrapper->addStatementBott(
        "where",
        "A [" + internalFunction + " B]");

  // make it accessible for others
  operator2wrapper.insert_or_assign(op + type, wrapper);
}

string TXLGenerator::getWrapperForOperator(const string& op, const string& type)
{
  if (auto const x = operator2wrapper.find(op + type); x != operator2wrapper.cend())
    return x->second->name;
  else
    SCIS_ERROR("Cant find suitable wrapper for operator \'" << op << "\' with type \'" << type << "\'");
}

void TXLGenerator::evaluateKeywordsDistances()
{
  maxDistanceToRoot.clear();

  deque<pair<string_view, int>> queue;
  for (auto const key : annotation->grammar.graph.topKeywords) {
    queue.push_back(make_pair(key->id, 0));

    // make first pass over top-most keywords work
    maxDistanceToRoot.insert_or_assign(key->id, -1);
  }

  auto iter = 0;
  while (!queue.empty() && iter++ < DAG_DISTANCES_SEARCH_LIMMIT) {
    auto const [candidate, newDistance] = std::move(queue.front());
    queue.pop_front();

    // check if there are any distance or not, aware of problems with std::string_view
    if (maxDistanceToRoot.find(candidate) == maxDistanceToRoot.cend())
      maxDistanceToRoot.insert_or_assign(candidate, -1);

    auto& oldDistance = maxDistanceToRoot[candidate];
    if (oldDistance < newDistance) {
      oldDistance = newDistance;

      for (auto const& node : annotation->grammar.graph.keywords[candidate]->subnodes)
        queue.push_back(make_pair<string_view, int>(node, newDistance + 1));
    }
  }

  SCIS_DEBUG("Distances:");
  for (auto const& [keyword, distance] : maxDistanceToRoot)
    cerr << keyword << " = " << distance << endl;

  // dumb way to check for loops
  if (iter >= DAG_DISTANCES_SEARCH_LIMMIT)
    SCIS_ERROR("Cycles detected in grammar annotation (see keyword-DAG)");
}

void TXLGenerator::loadRequestedFragments()
{
  FragmentParser parser;

  for (auto const& request : ruleset->fragments) {
    SCIS_DEBUG("Loading fragment from [" << request.path << "]");

    auto fragment = parser.parse(fragmentsDir + request.path + ".xml");
    fragments.insert_or_assign(fragment->name, std::move(fragment));
  }
}

void TXLGenerator::sortKeywords(vector<string_view>& keywords) const
{
  std::sort(keywords.begin(), keywords.end(),
            [&](string_view const& a, string_view const& b) {
      return maxDistanceToRoot.at(a) < maxDistanceToRoot.at(b);
    });
}

void TXLGenerator::compileContextCheckers()
{
  vector<BasicContext const*> basicContexts;
  vector<CompoundContext const*> compoundContexts;

  unordered_set<GrammarAnnotation::PointOfInterest const*> allPOIs;

  // collect all points of interest and classify + group contexts
  for (auto const& [_, context] : ruleset->contexts) {
    auto const ctx = context.get();

    if (auto const basicCtx = dynamic_cast<BasicContext const*>(ctx)) {
      basicContexts.push_back(basicCtx);

      // collecting points-of-interest
      for (auto const& constraint : basicCtx->constraints) {
        auto const poi = annotation->pointsOfInterest[constraint.id].get();
        allPOIs.insert(poi);
      }
    }
    else if (auto const compoundCtx = dynamic_cast<CompoundContext const*>(ctx))
      compoundContexts.push_back(compoundCtx);
    else
      SCIS_ERROR("Unrecognized context type");
  }

  // create getters
  for (auto const poi : allPOIs)
    compileGetterForPOI(poi);

  // create checking functions for basic contexts
  for (auto const ctx : basicContexts)
    compileBasicContext(ctx);

  deque<CompoundContext const*> deferredContextCheckers;
  // put all remaining contexts into queue
  deferredContextCheckers.insert(deferredContextCheckers.begin(), compoundContexts.begin(), compoundContexts.end());

  // process all
  auto iter = 0;
  while (!deferredContextCheckers.empty() && iter++ < CONTEXT_DEPENDENCY_WAITING_LIMMIT) {
    auto const context = std::move(deferredContextCheckers.front());
    deferredContextCheckers.pop_front();

    if (!compileCompoundContext(context))
      deferredContextCheckers.push_back(context);
  }

  // dumb way to check for loops
  if (iter >= CONTEXT_DEPENDENCY_WAITING_LIMMIT)
    SCIS_ERROR("Cycles detected in contexts");
}

void TXLGenerator::compileGetterForPOI(GrammarAnnotation::PointOfInterest const* const poi)
{
  auto const getter = createFunction<TXLFunction>();
  getter->isRule = false;
  getter->name = makeFunctionNameFromPOIName(poi->id);

  auto const keyword = annotation->grammar.graph.keywords[poi->keyword].get();
  auto& inputParam = getter->addParameter(makeNameFromKeyword(keyword->id), keyword->type);

  getter->addStatementTop(
        "replace [stringlit]",
        "_ [stringlit]");

  unordered_map<string, string> type2name;
  type2name.insert_or_assign(inputParam.type, inputParam.id);

  vector<string> path;
  path.push_back(inputParam.type);

  if (!poi->valueTypePath.empty())
    path.insert(path.end(), poi->valueTypePath.cbegin(), poi->valueTypePath.cend());

  for (size_t i = 0; i+1 < path.size(); i++) { // except last
    auto const& type = path[i];
    auto const& nextType = path[i + 1];

    stringstream pattern;
    // NOTE: only first variant used
    grammar->types[type]->variants[0].toTXLWithNames(pattern, [&](string const& processingType) {
        return type2name[processingType.data()] =
            processingType == nextType ?
              makeNameFromType(processingType) + to_string(i) :
              "_";
      });

    getter->deconstructVariable(type2name[type], pattern.str());
  }

  auto const& lastType = path.back();
  auto const result = type2name[lastType] + "_str";

  getter->createVariable(
        result, TXL_TYPE_STRING,
        "_ [quote " + type2name[lastType] + "]");

  getter->addStatementBott(
        "by",
        result);

  // make getter accessible for others
  poi2getter[poi] = getter;
}

TXLFunction* TXLGenerator::prepareContextChecker(Context const* const context,
                                                 bool const positive)
{
  auto const checker = createFunction<TXLFunction>();
  checker->isRule = false;
  checker->name = makeFunctionNameFromContextName(context->id, !positive);

  // always match
  checker->addStatementBott(
        "match [any]",
        "_ [any]");

  // create garbage variable as primary target for 'when' conditions
  checker->createVariable(
        NODE_VOID, NODE_VOID_TYPE,
        NODE_VOID_VALUE);

  return checker;
}

void TXLGenerator::registerContextChecker(Context const* const context,
                                          TXLFunction const* const checker)
{
  // register for later use
  // FIXME: check for context duplication
  contextCheckers.insert_or_assign(context->id, checker);
}

TXLFunction const* TXLGenerator::findContextCheckerByContext(string const& name)
{
  if (auto const c = contextCheckers.find(name); c != contextCheckers.cend())
    return c->second;
  else
    return nullptr;
}

void TXLGenerator::compileBasicContext(BasicContext const* const context)
{
  // check for dependencies -> there are no dependencies

  SCIS_DEBUG("Processing basic context <" << context->id << ">");

  // creating new context checking function (incl. VOID variable)
  auto const checker = prepareContextChecker(context, true);

  unordered_set<string_view> keywordsUsed;
  unordered_set<GrammarAnnotation::PointOfInterest const*> POIsUsed;
  for (size_t cNum = 0; cNum < context->constraints.size(); cNum++) {
    auto const& constraint = context->constraints[cNum];
    auto const poi = annotation->pointsOfInterest[constraint.id].get();

    // collect names for parameters and variables
    keywordsUsed.insert(poi->keyword);
    POIsUsed.insert(poi);
  }

  // keep parameters in order
  vector<string_view> sortedKeywords;
  sortedKeywords.insert(sortedKeywords.begin(), keywordsUsed.begin(), keywordsUsed.end());
  sortKeywords(sortedKeywords);

  // introduce parameters
  unordered_map<string, string> type2name;
  for (auto const& kw : sortedKeywords) {
    auto const& param = checker->addParameter(codegen::makeNameFromKeyword(kw),
                                              annotation->grammar.graph.keywords[kw]->type);
    type2name.insert_or_assign(param.type, param.id);
  }

  // instantiate variables which will keep values for POIs
  unordered_map<decltype(POIsUsed)::key_type, string> poi2var;
  for (auto const poi : POIsUsed) {
    auto const getter = poi2getter[poi];
    auto const varName = makeNameFromPOIName(poi->id) + "_str";
    checker->createVariable(
          varName, TXL_TYPE_STRING,
          "_ [" + getter->name + " " + type2name[getter->params[0].type] + "]");

    // save for future use
    poi2var.insert_or_assign(poi, varName);
  }

  // create "where" checks
  auto& where = checker->addStatementBott("where all", NODE_VOID);
  // joined with 'AND' operation
  for (auto const& constraint : context->constraints) {
    auto const poi = annotation->pointsOfInterest[constraint.id].get();
    auto const& property = poi2var[poi];
    auto const operation = getWrapperForOperator(constraint.op, TXL_TYPE_STRING);

    // append expressions (pre-fix notation)
    where.text += " [" + operation + " " + property + " \"" + constraint.value.text + "\"]";
    // TODO: string templates
  }

  // make it available for others (ie contexts and filtering functions)
  registerContextChecker(context, checker);

  // create negation form
  compileBasicContextNagation(context, checker);
}

void TXLGenerator::compileBasicContextNagation(Context const* const context,
                                               TXLFunction const* const contextChecker)
{
  auto const func = prepareContextChecker(context, false);
  // signature should be the same with the original context checker
  func->copyParamsFrom(contextChecker);

  string args = "";
  for (auto const& param : func->params)
    args += " " + param.id;

  func->addStatementBott(
        "where not",
        NODE_VOID + " [" + contextChecker->name + args + "]");
}

// BUG: check for context reference loops
bool TXLGenerator::compileCompoundContext(CompoundContext const* const context)
{
  unordered_set<string> requiredTypes;
  // check for dependencies
  for (auto const& disjunction : context->references)
    for (auto const& reference : disjunction) {
      // each reference can be just basic context or complicated compound context
      auto const contextChecker = findContextCheckerByContext(reference.id);
      if (!contextChecker)
        // failed to create a thing
        return false;

      // collect types required for context checks
      for (auto const& parameter : contextChecker->params)
        requiredTypes.insert(parameter.type);
    }

  SCIS_DEBUG("Processing compound context <" << context->id << ">");

  auto const checker = prepareContextChecker(context, true);

  unordered_map<string, string> type2name;
  // create parameters for this particular context checker
  for (auto const& type : requiredTypes) {
    auto const& param = checker->addParameter(makeNameFromType(type), type);
    type2name.insert_or_assign(type, param.id);
  }

  // joined with 'AND'
  for (auto const& disjunction : context->references) {
    auto& where = checker->addStatementBott("where", NODE_VOID);
    // joined with 'OR'
    for (auto const& reference : disjunction) {
      // BUG: global context reference in a compound context
      auto const target = makeFunctionNameFromContextName(reference.id, reference.isNegative);
      auto const ref = findContextCheckerByContext(reference.id);

      // map arguments to callee parameters
      string args = "";
      for (auto const& param : ref->params)
        args += " " + type2name[param.type];

      // actual 'call'
      where.text += " [" + target + args + "]";
    }
  }

  // make it available for others
  registerContextChecker(context, checker);

  // create negation form
  compileBasicContextNagation(context, checker);

  // everything done ok
  return true;
}

template<typename Kind>
Kind* TXLGenerator::createFunction()
{
  auto func = make_unique<Kind>();
  auto const function = func.get();
  functions.emplace_back(std::move(func));
  return function;
}

void TXLGenerator::compileCollectionFunctions(string const& ruleId,
                                              Context const* const context)
{
  unordered_set<string_view> keywordsUsedInContext;

  if (auto const compoundCtx = dynamic_cast<CompoundContext const*>(context))
    // collect keywords
    for (auto const& disjunction : compoundCtx->references)
      for (auto const& element : disjunction) {
        // FIXME: compound -> basic context resolution
        auto const ctxReference = ruleset->contexts[element.id].get();
        if (!ctxReference)
          SCIS_ERROR("Undefined reference to a context with id <" << element.id << ">");

        auto const basicCtx = dynamic_cast<BasicContext*>(ctxReference);

        // BUG: only first POI used
        auto const& constraint = basicCtx->constraints.front();
        auto const& poi = annotation->pointsOfInterest[constraint.id].get();

        keywordsUsedInContext.insert(poi->keyword);
      }
  else if (auto const basicCtx = dynamic_cast<BasicContext const*>(context)) {
    // BUG: only first POI used
    auto const& constraint = basicCtx->constraints.front();
    auto const& poi = annotation->pointsOfInterest[constraint.id].get();

    keywordsUsedInContext.insert(poi->keyword);
  }
  else
    SCIS_ERROR("Unknown context type");

  // sort keywords
  vector<string_view> sortedKeywords;
  sortedKeywords.insert(sortedKeywords.begin(), keywordsUsedInContext.begin(), keywordsUsedInContext.end());
  sortKeywords(sortedKeywords);

  SCIS_DEBUG("sorted keywords:");
  for (auto const& k : sortedKeywords)
    cout << k << endl;

  // create and add collection functions to a sequence
  for (auto const& keyword : sortedKeywords) {
    auto const cFunc = createFunction<CollectionFunction>();
    cFunc->isRule = true; // BUG: is collector always a rule?

    if (auto const lastCollector = dynamic_cast<CollectionFunction*>(lastCallChainElement())) {
      // add params of last collection function
      cFunc->copyParamsFrom(lastCollector);

      // plus one parameter (result) from last collection function
      cFunc->addParameter(makeNameFromKeyword(lastCollector->processingKeyword), lastCollector->processingType);

      // strong chaining
      //cFunc->skipType = lastCollector->processingType; // BUG: potential skipping bug
    }

    cFunc->name = ruleId + "_collector_" + context->id + "_" + cFunc->processingKeyword + to_string(__LINE__) + getUniqueId();
    cFunc->processingKeyword = keyword;
    cFunc->processingType = annotation->grammar.graph.keywords[keyword]->type;
    cFunc->skipType = cFunc->processingType; // BUG: potential skipping bug

    // hook-up together with everything else
    addToCallChain(cFunc);
  }
}

void TXLGenerator::compileFilteringFunction(string const& ruleId,
                                            Context const* const context)
{
  // add context filtering function
  auto const fFunc = createFunction<FilteringFunction>();

  // setup basic info
  fFunc->isRule = false;
  fFunc->name = ruleId + "_filter_" + context->id + to_string(__LINE__) + getUniqueId();
  auto const lastCollector = dynamic_cast<CollectionFunction*>(lastCallChainElement());
  fFunc->copyParamsFrom(lastCollector);

  fFunc->processingType = lastCollector->processingType;

  // plus one parameter (result) from last collection function
  fFunc->addParameter(makeNameFromKeyword(lastCollector->processingKeyword), lastCollector->processingType);

  unordered_map<string, string> type2name;
  // organize parameters
  for (auto const& par : fFunc->params)
    type2name.insert_or_assign(par.type, par.id);

  // create garbage variable
  fFunc->createVariable(
        NODE_VOID, NODE_VOID_TYPE,
        NODE_VOID_VALUE);

  // prepare to call context checker
  auto const checker = contextCheckers[context->id];

  string args = "";
  for (auto const& p : checker->params)
    args += " " + type2name[p.type];

  // actual checks (functions should already been created)
  fFunc->addStatementBott(
        "where",
        NODE_VOID + " [" + checker->name + args + "]");

  // hook-up together
  addToCallChain(fFunc);
}

void TXLGenerator::compileRefinementFunctions(
    string const& ruleId,
    Rule::Statement const& ruleStmt)
{
  using namespace std::placeholders;

  unordered_map<string, RefinementFunctionGenerator> GENERATORS {
    { "first",   bind(&TXLGenerator::compileRefinementFunction_First,          this, _1, _2, _3) },
    { "all",     bind(&TXLGenerator::compileRefinementFunction_All,            this, _1, _2, _3) },
    { "level",   bind(&TXLGenerator::compileRefinementFunction_Level,          this, _1, _2, _3) },
    { "level_p", bind(&TXLGenerator::compileRefinementFunction_LevelPredicate, this, _1, _2, _3) },
  };

  // add path functions
  auto index = 0;
  for (auto const& path : ruleStmt.location.path) {
    // TODO: modifiers for refiement functions
    auto const generator = GENERATORS.find(path.modifier);
    if (generator == GENERATORS.cend())
      SCIS_ERROR("Icorrect modifier near path element in rule <" << ruleId << ">");

    auto const name = ruleId + "_refiner_" + path.keywordId + "_" + path.modifier + to_string(index);
    generator->second(name, path, index);

    // update index
    if (index > maxRefinementIndex)
      maxRefinementIndex = index;

    ++index;
  }
}

void TXLGenerator::compileRefinementFunction_First(
    string const& name,
    Rule::Location::PathElement const& path,
    int const index)
{
  auto const keyword = annotation->grammar.graph.keywords[path.keywordId].get();
  auto const rFunc = createFunction<RefinementFunction_First>();
  rFunc->isRule = false;
  rFunc->name = name;
  rFunc->copyParamsFrom(lastCallChainElement());
  rFunc->processingType = keyword->type;
  rFunc->skipType = nullopt; // FIXME: refinement skipping
  rFunc->sequential = keyword->sequential;
  rFunc->queueIndex = index;

  // TODO: templates in refinement functions
  path.pattern;

  rFunc->searchType =
      keyword->sequential ?
        annotation->grammar.baseSequenceType :
        rFunc->processingType;

  // hook-up together
  addToCallChain(rFunc);
}

void TXLGenerator::compileRefinementFunction_All(
    string const& name,
    Rule::Location::PathElement const& path,
    int const index)
{
  auto const keyword = annotation->grammar.graph.keywords[path.keywordId].get();
  auto const SKIP = getSkipNodeName(index);

  if (keyword->sequential) {
    // kick-starter function
    auto const helper = createFunction<CallChainFunction>(); // TODO: move to a new class
    helper->isRule = false;
    helper->name = name + SUFFIX_HELPER;
    helper->copyParamsFrom(lastCallChainElement());
    helper->processingType = lastCallChainElement()->processingType;//annotation->grammar.baseSequenceType;

    helper->addStatementTop(
          "replace * [" + helper->processingType + "]",
          NODE_CURRENT + " [" + helper->processingType + "]");

    helper->exportVariableCreate(
          SKIP, TXL_TYPE_NUMBER,
          "0");

    helper->addStatementBott(
          "by",
          NODE_CURRENT + " [" + name + helper->getParamNames() + "]");

    addToCallChain(helper);
  }

  // actual work
  auto const rFunc = createFunction<RefinementFunction_All>();
  rFunc->isRule = true;
  rFunc->name = name;
  rFunc->copyParamsFrom(lastCallChainElement());
  rFunc->skipType = nullopt; // FIXME: refinement skipping
  rFunc->processingType = keyword->type;

  // type-dependent data
  rFunc->sequential = keyword->sequential;
  rFunc->queueIndex = index;
  rFunc->skipCount = SKIP;
  rFunc->skipCountDecrementer = getSkipDecrementerName(index);

  // TODO: templates in refinement functions
  path.pattern;

  rFunc->searchType =
      keyword->sequential ?
        annotation->grammar.baseSequenceType :
        rFunc->processingType;

  // hook-up together
  addToCallChain(rFunc);
}

void TXLGenerator::compileRefinementFunction_Level(
    string const& name,
    Rule::Location::PathElement const& path,
    int const index)
{
  auto const keyword = annotation->grammar.graph.keywords[path.keywordId].get();
  auto const SKIP = getSkipNodeName(index);

  // kick-starter function
  auto const helper = createFunction<CallChainFunction>(); // TODO: move to a new class
  helper->isRule = false;
  helper->name = name + SUFFIX_HELPER;
  helper->copyParamsFrom(lastCallChainElement());
  helper->processingType = lastCallChainElement()->processingType;

  helper->addStatementTop(
        "replace * [" + helper->processingType + "]",
        NODE_CURRENT + " [" + helper->processingType + "]");

  helper->exportVariableCreate(
        SKIP, TXL_TYPE_NUMBER,
        "0");

  helper->addStatementBott(
        "by",
        NODE_CURRENT + " [" + name + helper->getParamNames() + "]");

  addToCallChain(helper);

  // actual work
  auto const rFunc = createFunction<RefinementFunction_Level>();
  rFunc->isRule = true;
  rFunc->name = name;
  rFunc->copyParamsFrom(helper);
  rFunc->skipType = nullopt; // FIXME: refinement skipping
  rFunc->processingType = keyword->type;

  // type-dependent data
  rFunc->sequential = keyword->sequential;
  rFunc->queueIndex = index;
  rFunc->skipCount = SKIP;
  rFunc->skipCountDecrementer = getSkipDecrementerName(index);
  rFunc->skipCountCounter = getSkipCounterName(rFunc->name);

  // TODO: templates in refinement functions
  path.pattern;

  rFunc->searchType =
      keyword->sequential ?
        annotation->grammar.baseSequenceType :
        rFunc->processingType;

  // counter
  auto const counter = createFunction<TXLFunction>(); // TODO: move to a new class
  counter->name = getSkipCounterName(rFunc->name);
  counter->isRule = true;

  counter->addStatementTop(
        "replace $ [" + rFunc->processingType + "]",
        NODE_CURRENT + "[" + rFunc->processingType + "]");

  counter->importVariable(
        SKIP, TXL_TYPE_NUMBER);

  counter->exportVariableUpdate(
        SKIP,
        SKIP + " [+ 1]");

  counter->addStatementBott(
        "by",
        NODE_CURRENT);

  // hook-up together
  addToCallChain(rFunc);
}

void TXLGenerator::compileRefinementFunction_LevelPredicate(
    string const& name,
    Rule::Location::PathElement const& path,
    int const index)
{
  SCIS_ERROR("WIP " << __FUNCTION__);
}

void TXLGenerator::compileRefinementHelperFunctions()
{
  // 'nothing' action
  auto const doNothing = createFunction<TXLFunction>();
  doNothing->isRule = false;
  doNothing->name = ACTION_NOTHING;

  doNothing->addStatementTop(
        "match [any]",
        "_ [any]");

  // 'skip--' action
  for (auto index = 0; index <= maxRefinementIndex; index++) {
    auto const SKIP = getSkipNodeName(index);

    // function will subtract one (1) from 'SKIP' global variable
    auto const decrementer = createFunction<TXLFunction>();
    decrementer->isRule = false;
    decrementer->name = getSkipDecrementerName(index);

    decrementer->addStatementTop(
          "match [any]",
          "_ [any]");

    decrementer->importVariable(
          SKIP,
          TXL_TYPE_NUMBER);

    decrementer->addStatementBott(
          "where",
          SKIP + " [> 0]");

    decrementer->exportVariableUpdate(
          SKIP,
          SKIP + " [- 1]");
  }
}

string TXLGenerator::getSkipNodeName(const int index)
{
  return PREFIX_NODE_SKIP + to_string(index);
}

string TXLGenerator::getSkipDecrementerName(int const index)
{
  return PREFIX_STD + "decrement_skip" + to_string(index);
}

string TXLGenerator::getSkipCounterName(const string& refiner)
{
  return refiner + "_red_box_counter";
}

void TXLGenerator::compileInstrumentationFunction(
    string const& ruleId,
    Rule::Statement const& ruleStmt,
    Context const* const context)
{
  // add instrumentation function
  auto const iFunc = createFunction<InstrumentationFunction>();
  auto const keyword = annotation->grammar.graph.keywords[ruleStmt.location.path.back().keywordId].get(); // BUG: empty path?
  auto const& algo = keyword->pointcuts[ruleStmt.location.pointcut]->aglorithm;
  auto const& pattern = keyword->replacement_patterns[0]; // BUG: only first pattern variant used
  auto const& workingType = keyword->type;

  // setup basic info
  iFunc->isRule = false;
  iFunc->name = ruleId + "_instrummenter_" + ruleStmt.location.pointcut + to_string(__LINE__) + getUniqueId();

  auto const lastFunc = lastCallChainElement();
  iFunc->copyParamsFrom(lastFunc);
  iFunc->searchType = pattern.searchType;
  iFunc->processingType = lastFunc->processingType;

  // deconstruct current node if possible
  if (auto const type = grammar->types.find(workingType); type != grammar->types.cend()) {
    stringstream pattern;
    // BUG: only first variant used
    type->second->variants[0].toTXLWithNames(pattern, makeNameFromType);

    iFunc->deconstructVariable(NODE_CURRENT, pattern.str());
  }
  else
    SCIS_WARNING("Cant deconstruct type [" << workingType << "]");

  // introduce common variables and constants
  unordered_map<string, pair<string, string>> const PREDEFINED_IDENTIFIERS {
    {"NODE",     {TXL_TYPE_ID,     "_ [typeof " + NODE_CURRENT + "]"}},
    {"POINTCUT", {TXL_TYPE_ID,     '\'' + ruleStmt.location.pointcut}},
    {"FILE",     {TXL_TYPE_STRING, "_ [+ \"" + processingFilename + "\"]"}},
  };

  for (auto const& [name, typeAndValue] : PREDEFINED_IDENTIFIERS) {
    auto const& [type, value] = typeAndValue;
    iFunc->createVariable(name, type, value);
  }

  context;// FIXME: add variables from POI

  // ==========

  unordered_set<string_view> syntheticVariables;
  for (auto const& make : ruleStmt.actionMake) {
    stringstream sequence;
    sequence << "_ ";
    for (auto const& component : make.components)
      component->toTXL(sequence);

    iFunc->createVariable(
          make.target, TXL_TYPE_STRING,
          sequence.str());

    syntheticVariables.insert(make.target);
  }

  // ==========

  for (auto const& add : ruleStmt.actionAdd) {
    // ----

    auto const fragment = getFragment(add.fragmentId);
    // TODO: check dependencies
    if (fragment->language != annotation->grammar.language)
      SCIS_ERROR("Missmatched languages of fragment <" << add.fragmentId << "> and annotation");

    // check if all arguments actualy exist
    for (auto const& arg : add.args)
      if (syntheticVariables.find(arg) == syntheticVariables.cend())
        SCIS_ERROR("Variable <" << arg << "> not found");

    auto const preparedFragment = prepareFragment(fragment, add.args);
    // ----

    string replacement = "";
    for (auto const& block : pattern.blocks) {
      auto const ptr = block.get();

      if (auto txt = dynamic_cast<GrammarAnnotation::Pattern::TextBlock*>(ptr))
        replacement += txt->text + ' ';

      else
        if (auto ref = dynamic_cast<GrammarAnnotation::Pattern::TypeReference*>(ptr)) {
          replacement += makeNameFromType(ref->typeId) + ' ';
          // TODO: check if type exists in a current node
        }
      else
        if (auto pt = dynamic_cast<GrammarAnnotation::Pattern::PointcutLocation*>(ptr)) {
          // TODO: add pointcut name wildcard
          if (pt->name == ruleStmt.location.pointcut) {
            string algorithmResult;

            auto const handler = [&](FunctionCall::Result const& result) {
              if (!result.byText.empty())
                algorithmResult += result.byText + " ";
            };

            // execute algorithm
            for (auto const& step : algo)
              callAlgorithmCommand({
                                     .function = step.function,
                                     .args = step.args,
                                     .preparedFragment = preparedFragment,
                                     .iFunc = iFunc,
                                     .grammar = grammar.get()
                                   },
                                   handler);

            // append result
            replacement += algorithmResult + " ";
          }
        }
      else
        SCIS_ERROR("Undefined pattern block");
    }

    iFunc->replacement = replacement;
  }

  // hook-up together
  addToCallChain(iFunc);
}

string TXLGenerator::prepareFragment(Fragment const* const fragment,
                                     vector<string> const& args)
{
  stringstream ss;
  fragment->toTXL(ss, args);
  return ss.str();
}

void TXLGenerator::createMain()
{
  mainFunc = createFunction<TXLFunction>();
  mainFunc->isRule = false;
  mainFunc->name = "main";
}

void TXLGenerator::compileMain()
{
  mainFunc->addStatementTop(
        "replace [program]",
        "Program [program]");

  string callSeq = "";
  for (auto const& function : mainCallSequence)
    callSeq += "\t\t[" + function->name + "]\n";

  mainFunc->addStatementBott(
        "by",
        "Program\n" + callSeq);
}

void TXLGenerator::compileAnnotationUtilityFunctions()
{
  for (auto const& func : annotation->library) {
    auto const function = createFunction<TXLFunction>();
    function->isRule = func->isRule;
    function->name = func->name;

    for (auto const& parameter : func->params)
      function->addParameter(parameter.id, parameter.type);

    function->addStatementBott(
          func->source,
          "% copied from annotation");

    // apply call policy
    switch (func->callPolicy) {
      case GrammarAnnotation::FunctionPolicy::DIRECT_CALL:
        // do nothing
        break;

      case GrammarAnnotation::FunctionPolicy::BEFORE_ALL:
        mainCallSequence.push_front(function);
        break;

      case GrammarAnnotation::FunctionPolicy::AFTER_ALL:
        mainCallSequence.push_back(function);
        break;

      default:
        SCIS_ERROR("Unsupported function call policy");
    }
  }
}

void TXLGenerator::compileStandardWrappers(string const& baseType)
{
  for (auto const& [op, names] : OPERATOR_WRAPPER_MAPPING) {
    auto const& [internalFunction, wrapperName] = names;
    wrapStandardBinnaryOperator(op, baseType, wrapperName, internalFunction);
  }
}

void TXLGenerator::genTXLImports(ostream& str)
{
  str << "include \"" << annotation->grammar.txlSourceFilename << '\"' << endl;
}

// NOTE: assumption: conjunctions are correct and contain references only to basic contexts
void TXLGenerator::compile()
{
  loadRequestedFragments();

  evaluateKeywordsDistances();

  createMain();

  compileStandardWrappers(TXL_TYPE_STRING);

  compileContextCheckers();

  for (auto const& [_, rule] : ruleset->rules) {
    SCIS_DEBUG("Processing rule \'" << rule->id << "\'");

    for (auto const& ruleStmt : rule->statements) {
      SCIS_DEBUG("Processing statement");

      bool const isGlobalContext = (ruleStmt.location.contextId == "@");

      auto const context =
          isGlobalContext ?
            GLOBAL_CONTEXT.get() :
            ruleset->contexts[ruleStmt.location.contextId].get();

      // generate chain of functions
      if (!isGlobalContext) {
        SCIS_DEBUG("compiling Collectors...");
        compileCollectionFunctions(rule->id, context);

        SCIS_DEBUG("compiling Filter...");
        compileFilteringFunction(rule->id, context);
      }

      SCIS_DEBUG("compiling Path*...");
      compileRefinementFunctions(rule->id, ruleStmt);

      SCIS_DEBUG("compiling Instrumenter...");
      compileInstrumentationFunction(rule->id, ruleStmt, context);

      // add to a whole program
      mainCallSequence.push_back(currentCallChain.front());
      resetCallChain();
    }
  }

  compileRefinementHelperFunctions();

  compileAnnotationUtilityFunctions();

  compileMain();
}

void TXLGenerator::generateCode(ostream& str)
{
  SCIS_DEBUG("Generating TXL sources");

  genTXLImports(str);

  // generate all functions including utilities and main
  for (auto const& func : functions) {
    str << endl;
    func->generateTXL(str);
    str << endl;
  }
}
