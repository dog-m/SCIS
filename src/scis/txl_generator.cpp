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

  if (iter >= DAG_DISTANCES_SEARCH_LIMMIT)
    SCIS_ERROR("Cycle detected in grammar annotation (see keyword-DAG)");
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

  std::sort(sortedKeywords.begin(), sortedKeywords.end(),
            [&](string_view a, string_view b) {
      return maxDistanceToRoot[a] < maxDistanceToRoot[b];
    });

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
      auto& cParam = cFunc->params.emplace_back(/* empty */);
      cParam.id = keywordToName(lastCollector->processingKeyword);
      cParam.type = lastCollector->processingType;

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
  auto& fParam = fFunc->params.emplace_back(/* empty */);
  fParam.id = keywordToName(lastCollector->processingKeyword);
  fParam.type = lastCollector->processingType;

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

        compileBasicContext(basicCtx, element.negation, fFunc, type2name);
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

  // create string variable
  if (poi->valueTypePath.empty()) {
    auto const& targetType = annotation->grammar.graph.keywords[poi->keyword]->type;
    where.target = type2name[targetType] + "_str";

    fFunc->createVariable(where.target, "stringlit", "_ [quote " + type2name[targetType] + "]");
  }
  else {
    auto const& topType = annotation->grammar.graph.keywords[poi->keyword]->type;
    vector<string> path;
    path.insert(path.begin(), poi->valueTypePath.cbegin(), poi->valueTypePath.cend());
    path.insert(path.begin(), topType);

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

  for (auto const& chain : callTree)
    byStmt.text += "\t\t[" + chain.front()->name + "]\n";
}

void TXLGenerator::genUtilityFunctions(ostream& str)
{
  // FIXME: utility function call policy ignored

  for (auto const& function : annotation->library) {
    auto const kind = function->isRule ? "rule" : "function";
    str << endl << kind << ' ' << function->name;

    for (auto const& parameter : function->params)
      str << ' ' << parameter.id << " [" << parameter.type << "]";
    str << endl;

    str << '\t' << function->source << endl;

    str << "end " << kind << endl;
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
      callTree.emplace_back(std::move(currentCallChain));
    }
  }

  compileMain();
}

void TXLGenerator::generateCode(ostream& str)
{
  SCIS_DEBUG("Generating TXL sources");

  genTXLImports(str);

  // generate utility functions
  genUtilityFunctions(str);

  // generate all chains
  for (auto const& chain : callTree)
    for (auto const& func : chain) {
      str << endl;
      func->generateTXL(str);
      str << endl;
    }

  // generate main function
  mainFunction->generateTXL(str);
}
