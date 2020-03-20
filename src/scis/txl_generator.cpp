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

static unordered_map<string_view, string_view> CTX_OP_INVERSION_MAPPING {
  { "=", "~=" },
  { "~=", "=" },
  { "<", ">=" },
  { "<=", ">" },
  { ">", "<=" },
  { ">=", "<" },
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

string TXLGenerator::keywordToName(string const& keyword)
{
  return "kw_" + makeNameFromType(keyword);
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
  deque<Context const*> deferredContextCheckers;
  // put all contexts into queue
  for (auto const& [_, context] : ruleset->contexts)
    deferredContextCheckers.push_back(context.get());

  // process all
  auto iter = 0;
  while (!deferredContextCheckers.empty() && iter++ < CONTEXT_DEPENDENCY_WAITING_LIMMIT) {
    auto const context = std::move(deferredContextCheckers.front());
    deferredContextCheckers.pop_front();

    bool success = true;

    if (auto const basicCtx = dynamic_cast<BasicContext const*>(context))
      success = compileBasicContext(basicCtx);
    else if (auto const compoundCtx = dynamic_cast<CompoundContext const*>(context))
      success = compileCompoundContext(compoundCtx);
    else
      SCIS_ERROR("Unrecognized context type");

    if (!success)
      deferredContextCheckers.push_back(context);
  }

  // dumb way to check for loops
  if (iter >= CONTEXT_DEPENDENCY_WAITING_LIMMIT)
    SCIS_ERROR("Cycles detected in contexts");
}

TXLFunction* TXLGenerator::prepareContextChecker(const Context* const context)
{
  auto const checker = createFunction<TXLFunction>();
  checker->name = contextNameToFunctionName(context->id);

  // always match
  auto& mStmt = checker->statements.emplace_back(/* empty */);
  mStmt.action = "match [any]";
  mStmt.text = "_ [any]";

  // create garbage variable as primary target for 'when' conditions
  checker->createVariable(VOID_NODE, VOID_NODE_TYPE, VOID_NODE_VALUE);

  return checker;
}

void TXLGenerator::registerContextChecker(Context const* const context,
                                          TXLFunction const* const checker)
{
  // register for later use
  // FIXME: check for context duplication
  contextCheckers.insert_or_assign(context->id, checker);
}

TXLFunction const* TXLGenerator::findContextCheckerByContextName(string const& name)
{
  if (auto const c = contextCheckers.find(name); c != contextCheckers.cend())
    return c->second;
  else
    return nullptr;
}

bool TXLGenerator::compileBasicContext(BasicContext const* const context)
{
  // check for dependencies
  // there are no dependencies

  SCIS_DEBUG("Processing basic context <" << context->id << ">");

  // creating new context checking function
  auto const checker = prepareContextChecker(context);

  unordered_set<string_view> keywordsUsed;
  // joined with 'AND' operation
  for (size_t cNum = 0; cNum < context->constraints.size(); cNum++) {
    auto const& constraint = context->constraints[cNum];
    auto const& poi = annotation->pointsOfInterest[constraint.id].get();
    // collect names for parameters
    keywordsUsed.insert(poi->keyword);

    auto const& keyword = annotation->grammar.graph.keywords[poi->keyword].get();

    // use constraint number to generate unique namesa for variables
    cNum; keyword->type; poi->valueTypePath;
    constraint.op; constraint.value;
  }

  // TODO: create negation form of checker or something

  vector<string_view> sortedKeywords;
  sortedKeywords.insert(sortedKeywords.begin(), keywordsUsed.begin(), keywordsUsed.end());
  sortKeywords(sortedKeywords);

  sortedKeywords; checker->params;

  registerContextChecker(context, checker);
  return true;
}

// BUG: check for context reference loops
bool TXLGenerator::compileCompoundContext(CompoundContext const* const context)
{
  // check for dependencies
  for (auto const& disjunction : context->references)
    for (auto const& reference : disjunction)
      if (!findContextCheckerByContextName(reference.id))
        return false;

  SCIS_DEBUG("Processing compound context <" << context->id << ">");

  auto const checker = prepareContextChecker(context);

  // joined with 'AND'
  for (auto const& disjunction : context->references) {
    // joined with 'OR'
    for (auto const& reference : disjunction) {
      auto const target = contextNameToFunctionName(reference.id, reference.isNegative);
      auto const ref = findContextCheckerByContextName(reference.id);

      target;
      ref;
    }
  }

  registerContextChecker(context, checker);
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

    if (!currentCallChain.empty()) {
      auto const lastCollector = dynamic_cast<CollectionFunction*>(currentCallChain.back());

      // add params of last collection function
      cFunc->copyParamsFrom(lastCollector);

      // plus one parameter (result) from last collection function
      cFunc->addParameter(keywordToName(lastCollector->processingKeyword), lastCollector->processingType);

      // strong chaining
      //cFunc->skipType = lastCollector->processingType; // BUG: potential skipping bug
    }

    cFunc->name = ruleId + "_collector_" + context->id + "_" + cFunc->processingKeyword + to_string(__LINE__) + getUniqueId();
    cFunc->processingKeyword = keyword;
    cFunc->processingType = annotation->grammar.graph.keywords[keyword]->type;
    cFunc->skipType = cFunc->processingType; // BUG: potential skipping bug

    addToCallChain(cFunc);
  }
}

void TXLGenerator::compileFilteringFunction(string const& ruleId,
                                            Context const* const context)
{
  // add context filtering function
  auto const fFunc = createFunction<FilteringFunction>();

  fFunc->name = ruleId + "_filter_" + context->id + to_string(__LINE__) + getUniqueId();
  fFunc->copyParamsFrom(currentCallChain.back());

  // plus one parameter (result) from last collection function
  auto const lastCollector = dynamic_cast<CollectionFunction*>(currentCallChain.back());
  fFunc->addParameter(keywordToName(lastCollector->processingKeyword), lastCollector->processingType);

  unordered_map<string, string> type2name; // FIXME: introduce variables once
  for (auto const& par : fFunc->params)
    type2name.insert_or_assign(par.type, par.id);

  fFunc->processingType = lastCollector->processingType;

  ;// FIXME: !!! incorrect process understanding !!!

  if (auto const compoundCtx = dynamic_cast<CompoundContext const*>(context))
    for (auto const& disjunction : compoundCtx->references)
      for (auto const& element : disjunction) {
        // FIXME: compound -> basic context resolution
        auto const basicCtx = dynamic_cast<BasicContext*>(ruleset->contexts[element.id].get());

        compileBasicContext(basicCtx, element.isNegative, fFunc, type2name);
      }

  else
    if (auto const basicCtx = dynamic_cast<BasicContext const*>(context))
      compileBasicContext(basicCtx, false, fFunc, type2name);

  else
    SCIS_ERROR("Unrecognized context type");

  addToCallChain(fFunc);
}

void TXLGenerator::compileBasicContext(BasicContext const* const context,
                                       bool const topLevelNegation,
                                       FilteringFunction *const fFunc,
                                       unordered_map<string, string>& type2name)
{
  // BUG: only first POI used
  auto const& constraint = context->constraints.front();

  auto& where = fFunc->wheres.emplace_back(/* empty */);
  auto const poi = annotation->pointsOfInterest[constraint.id].get();

  {
    deque<string> path;
    if (!poi->valueTypePath.empty())
      path.insert(path.begin(), poi->valueTypePath.cbegin(), poi->valueTypePath.cend());

    path.push_front(annotation->grammar.graph.keywords[poi->keyword]->type);

    for (size_t i = 0; i+1 < path.size(); i++) { // except last
      auto const& type = path[i];

      stringstream pattern;
      // FIXME: only first variant used
      grammar->types[type]->variants[0].toTXLWithNames(pattern, [&](string_view const& name) {
          // BUG: variable name duplication
          return type2name[name.data()] =
              name == path[i + 1] ?
                makeNameFromType(name) :
                "_";
        });

      fFunc->deconstructVariable(type2name[type], pattern.str());
    }

    auto const& lastType = path.back();
    where.target = type2name[lastType] + "_str";

    fFunc->createVariable(where.target, "stringlit", "_ [quote " + type2name[lastType] + "]");
  }

  // FIXME: case: different context constraints for the same node
  auto& call = where.operators.emplace_back(/* empty */);
  call.name = topLevelNegation ? CTX_OP_INVERSION_MAPPING[constraint.op] : constraint.op;

  // FIXME: string templates
  call.args.push_back('\"' + constraint.value.text + '\"');
}

void TXLGenerator::compileRefinementFunctions(string const& ruleId,
                                              Rule::Statement const& ruleStmt)
{
  // add path functions
  for (auto const& path : ruleStmt.location.path) {
    auto const rFunc = createFunction<RefinementFunction>();

    // TODO: modifiers for refiement functions
    if (path.modifier == "all" || path.modifier == "first")
      rFunc->isRule = (path.modifier == "all");
    else
      SCIS_ERROR("Icorrect modifier near path element in rule <" << ruleId << ">");

    rFunc->name = ruleId + "_refiner_" + path.keywordId + to_string(__LINE__) + getUniqueId();
    if (!currentCallChain.empty())
      rFunc->copyParamsFrom(currentCallChain.back());

    rFunc->skipType = annotation->grammar.graph.keywords[path.keywordId]->type;
    rFunc->processingType = rFunc->skipType.value();

    // FIXME: templates in refinement functions
    path.pattern;

    addToCallChain(rFunc);
  }
}

void TXLGenerator::compileInstrumentationFunction(string const& ruleId,
                                                  Rule::Statement const& ruleStmt,
                                                  Context const* const context)
{
  // add instrumentation function
  auto const iFunc = createFunction<InstrumentationFunction>();
  auto const keyword = annotation->grammar.graph.keywords[ruleStmt.location.path.back().keywordId].get(); // BUG: empty path?
  auto const& algo = keyword->pointcuts[ruleStmt.location.pointcut]->aglorithm;
  auto const& pattern = keyword->replacement_patterns[0]; // BUG: only first pattern variant used
  auto const& workingType = keyword->type;

  iFunc->name = ruleId + "_instrummenter_" + ruleStmt.location.pointcut + to_string(__LINE__) + getUniqueId();
  iFunc->copyParamsFrom(currentCallChain.back());
  iFunc->searchType = pattern.searchType;
  iFunc->processingType = currentCallChain.back()->processingType;

  // deconstruct current node if possible
  if (auto const type = grammar->types.find(workingType); type != grammar->types.cend()) {
    stringstream pattern;
    // BUG: only first variant used
    type->second->variants[0].toTXLWithNames(pattern, makeNameFromType);

    iFunc->deconstructVariable(CURRENT_NODE, pattern.str());
  }
  else
    SCIS_WARNING("Cant deconstruct type [" << workingType << "]");

  // introduce common variables and constants
  iFunc->createVariable("NODE", "id", "_ [typeof " + CURRENT_NODE + "]");

  iFunc->createVariable("POINTCUT", "id", '\'' + ruleStmt.location.pointcut);

  iFunc->createVariable("FILE", "stringlit", "_ [+ \"" + processingFilename + "\"]");

  context;// FIXME: add variables from POI

  // ==========

  unordered_set<string_view> syntheticVariables;
  for (auto const& make : ruleStmt.actionMake) {
    stringstream ss;
    for (auto const& component : make.components)
      component->toTXL(ss);
    iFunc->createVariable(make.target, "stringlit", "_ " + ss.str());

    syntheticVariables.insert(make.target);
  }

  // ==========

  for (auto const& add : ruleStmt.actionAdd) {
    // ----

    auto const fragment = getFragment(add.fragmentId);
    // FIXME: check dependencies
    if (fragment->language != annotation->grammar.language)
      SCIS_ERROR("Missmatched languages of fragment <" << add.fragmentId << "> and annotation");

    // check if all arguments actualy exist
    for (auto const& arg : add.args)
      if (syntheticVariables.find(arg) == syntheticVariables.cend())
        SCIS_ERROR("Variable <" << arg << "> not found");

    stringstream ss;
    fragment->toTXL(ss, add.args);
    auto const preparedFragment = ss.str();

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

  addToCallChain(iFunc);
}

void TXLGenerator::compileMain()
{
  mainFunction = createFunction<TXLFunction>();
  mainFunction->name = "main";

  auto& replaceStmt = mainFunction->statements.emplace_back(/* empty */);
  replaceStmt.action = "replace [program]";
  replaceStmt.text = "Program [program]";

  auto& byStmt = mainFunction->statements.emplace_back(/* empty */);
  byStmt.action = "by";
  byStmt.text = "Program\n";

  for (auto const& function : addToMain)
    byStmt.text += "\t\t[" + function->name + "]\n";
}

void TXLGenerator::compileUtilityFunctions()
{
  // FIXME: utility function call policy ignored

  for (auto const& func : annotation->library) {
    auto const function = createFunction<TXLFunction>();
    function->isRule = func->isRule;
    function->name = func->name;

    for (auto const& p : func->params)
      function->addParameter(p.id, p.type);

    auto& stmt = function->statements.emplace_back(/* empty */);
    stmt.action = func->source;
    stmt.text = "% copied from annotation";

    // apply call policy
    switch (func->callPolicy) {
      case GrammarAnnotation::FunctionPolicy::DIRECT_CALL:
        // do nothing
        break;

      case GrammarAnnotation::FunctionPolicy::BEFORE_ALL:
        addToMain.push_front(function);
        break;

      case GrammarAnnotation::FunctionPolicy::AFTER_ALL:
        addToMain.push_back(function);
        break;

      default:
        SCIS_ERROR("Unsupported function call policy");
    }
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
      addToMain.push_back(currentCallChain.front());
      currentCallChain.clear();
    }
  }

  compileUtilityFunctions();

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
