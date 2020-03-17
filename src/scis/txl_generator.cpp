#include "txl_generator.h"
#include "logging.h"

#include <functional>
#include <deque>
#include <unordered_set>
#include <sstream>

#include "algorithm_commands.h"

using namespace std;
using namespace scis;

static string const CURRENT_NODE = "__NODE__";

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

  else {
    SCIS_ERROR("Unknown fragment <" << id << ">");
    return nullptr;
  }
}

void TXLGenerator::addToCallChain(unique_ptr<CallChainFunction>&& func)
{
  if (!currentCallChain.empty())
    currentCallChain.back()->connectTo(func.get());

  currentCallChain.emplace_back(std::move(func));
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

  constexpr auto maxIter = 2500;
  auto iter = 0;
  while (!queue.empty() && iter++ < maxIter) {
    auto const [candidate, newDistance] = std::move(queue.front());
    queue.pop_front();

    // check if there are any distance or not, aware of problems with std::string_view
    if (maxDistanceToRoot.find(candidate) == maxDistanceToRoot.cend())
      maxDistanceToRoot.insert_or_assign(candidate, -1);

    auto& oldDistance = maxDistanceToRoot[candidate];
    if (oldDistance < newDistance) {
      oldDistance = newDistance;

      SCIS_DEBUG("annotation->grammar.graph.keywords[ " << candidate << " ]: " << annotation->grammar.graph.keywords[candidate].get());

      for (auto const& node : annotation->grammar.graph.keywords[candidate]->subnodes)
        queue.push_back(make_pair<string_view, int>(node, newDistance + 1));
    }
  }

  SCIS_DEBUG("Distances:");
  for (auto const& [keyword, distance] : maxDistanceToRoot)
    SCIS_DEBUG(keyword << " = " << distance);

  if (iter > maxIter)
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

void TXLGenerator::compileCollectionFunctions(string const& ruleId,
                                              Context const* const context)
{
  unordered_set<string_view> keywordsUsedInContext;

  if (auto const compoundCtx = dynamic_cast<CompoundContext const*>(context))
    // collect keywords
    for (auto const& disjunction : compoundCtx->references)
      for (auto const& element : disjunction) {
        // FIXME: compound -> basic context resolution
        auto const basicCtx = dynamic_cast<BasicContext*>(ruleset->contexts.at(element.id).get());

        // BUG: only first POI used
        auto const& constraint = basicCtx->constraints.front();
        auto const& poi = annotation->pointsOfInterest.at(constraint.id).get();

        keywordsUsedInContext.insert(poi->keyword);
      }
  else if (auto const basicCtx = dynamic_cast<BasicContext const*>(context)) {
    // BUG: only first POI used
    auto const& constraint = basicCtx->constraints.front();
    auto const& poi = annotation->pointsOfInterest.at(constraint.id).get();

    keywordsUsedInContext.insert(poi->keyword);
  }
  else
    SCIS_ERROR("Unknown context type");

  // sorting keywords
  vector<string_view> sortedKeywords(keywordsUsedInContext.begin(), keywordsUsedInContext.end());
  std::sort(sortedKeywords.begin(), sortedKeywords.end(), [&](string_view a, string_view b) {
    SCIS_DEBUG("a: " << a.data() << "\t\t| b:" << b.data());

      return maxDistanceToRoot.at(a) < maxDistanceToRoot.at(b);
    });

  SCIS_DEBUG("sorted keywords:");
  for (auto const& k : sortedKeywords)
    cout << k << endl;

  // create and add collection functions to a sequence
  for (auto const& keyword : sortedKeywords) {
    auto cFunc = make_unique<CollectionFunction>();
    cFunc->isRule = true; // BUG: is collector always a rule?

    if (!currentCallChain.empty()) {
      auto const lastCollector = dynamic_cast<CollectionFunction*>(currentCallChain.back().get());

      // add params of last collection function
      cFunc->copyParamsFrom(lastCollector);

      // plus one parameter (result) from last collection function
      auto& cParam = cFunc->params.emplace_back(/* empty */);
      cParam.id = lastCollector->processingKeyword;
      cParam.type = lastCollector->processingType;

      // strong chaining
      //cFunc->skipType = lastCollector->processingType; // BUG: potential skipping bug
    }

    cFunc->name = ruleId + "_collector_" + context->id + "_" + cFunc->processingKeyword + to_string(__LINE__);
    cFunc->processingKeyword = keyword;
    cFunc->processingType = annotation->grammar.graph.keywords.at(keyword)->type;
    cFunc->skipType = cFunc->processingType; // BUG: potential skipping bug

    addToCallChain(std::move(cFunc));
  }
}

void TXLGenerator::compileFilteringFunction(string const& ruleId,
                                            Context const* const context)
{
  // add context filtering function
  auto fFunc = make_unique<FilteringFunction>();

  fFunc->name = ruleId + "_filter_" + context->id + to_string(__LINE__);
  fFunc->copyParamsFrom(currentCallChain.back().get());

  // plus one parameter (result) from last collection function
  auto const lastCollector = dynamic_cast<CollectionFunction*>(currentCallChain.back().get());
  auto& fParam = fFunc->params.emplace_back(/* empty */);
  fParam.id = lastCollector->processingKeyword;
  fParam.type = lastCollector->processingType;

  unordered_map<string, string> fVarName; // FIXME: introduce variables once
  for (auto const& par : fFunc->params)
    fVarName.insert(make_pair(par.type, par.id));

  fFunc->processingType = lastCollector->processingType;

  if (auto const compoundCtx = dynamic_cast<CompoundContext const*>(context))
    for (auto const& disjunction : compoundCtx->references)
      for (auto const& element : disjunction) {
        // FIXME: compound -> basic context resolution
        auto const basicCtx = dynamic_cast<BasicContext*>(ruleset->contexts.at(element.id).get());

        // BUG: only first POI used
        auto const& constraint = basicCtx->constraints.front();

        auto& where = fFunc->wheres.emplace_back(/* empty */);
        auto const poi = annotation->pointsOfInterest[constraint.id].get();

        // create string variable
        if (poi->valueTypePath.empty()) {
          auto const& targetType = annotation->grammar.graph.keywords[poi->keyword]->type;
          where.target = fVarName[targetType] + "_str";

          auto& constructStmt = fFunc->statements.emplace_back(/* empty */);
          constructStmt.action = "construct " + where.target + " [stringlit]";
          constructStmt.text = "_ [quote " + fVarName[targetType] + "]";
        }
        else {
          auto const& topType = annotation->grammar.graph.keywords[poi->keyword]->type;
          vector<string> path;
          path.insert(path.begin(), poi->valueTypePath.cbegin(), poi->valueTypePath.cend());
          path.insert(path.begin(), topType);

          for (auto const& type : path) {
            // skip last
            if (type == path.back())
              break;

            auto& deconstructStmt = fFunc->statements.emplace_back(/* empty */);
            deconstructStmt.action = "deconstruct " + fVarName[type];

            stringstream pattern;
            grammar->types[type]->variants[0].toTXLWithNames(pattern, [&](string_view const& name) {
                return fVarName[name.data()] = typeToName(name);
                // BUG: variable name duplication
              });

            deconstructStmt.text = pattern.str();
          }

          auto const& lastType = path.back();
          where.target = fVarName[lastType] + "_str";

          auto& constructStmt = fFunc->statements.emplace_back(/* empty */);
          constructStmt.action = "construct " + lastType + " [stringlit]";
          constructStmt.text = "_ [quote " + fVarName[lastType] + "]";
        }

        // FIXME: case: different context constraints for the same node
        auto& call = where.operators.emplace_back(/* empty */);
        call.name = element.negation ? CTX_OP_INVERSION_MAPPING[constraint.op] : constraint.op;
        // FIXME: string templates
        call.args.push_back(constraint.value.text);
      }

  else
    SCIS_ERROR("Filtering for basic contexts not implemented");// FIXME: missing basic contexts in filters

  addToCallChain(std::move(fFunc));
}

void TXLGenerator::compileRefinementFunctions(string const& ruleId,
                                              Rule::Statement const& ruleStmt)
{
  // add path functions
  for (auto const& path : ruleStmt.location.path) {
    auto rFunc = make_unique<RefinementFunction>();

    // TODO: modifiers for refiement functions
    if (path.modifier == "all" || path.modifier == "first")
      rFunc->isRule = (path.modifier == "all");
    else
      SCIS_ERROR("Icorrect modifier near path element in rule <" << ruleId << ">");

    rFunc->name = ruleId + "_refiner_" + path.keywordId + getUniqueId() + to_string(__LINE__);
    rFunc->copyParamsFrom(currentCallChain.back().get());

    rFunc->skipType = annotation->grammar.graph.keywords[path.keywordId]->type;
    rFunc->processingType = rFunc->skipType.value();

    // FIXME: templates in refinement functions
    path.pattern;

    addToCallChain(std::move(rFunc));
  }
}

void TXLGenerator::compileInstrumentationFunction(string const& ruleId,
                                                  Rule::Statement const& ruleStmt,
                                                  Context const* const context)
{
  // add instrumentation function
  auto iFunc = make_unique<InstrumentationFunction>();
  auto const keyword = annotation->grammar.graph.keywords[ruleStmt.location.path.back().keywordId].get(); // BUG: empty path?
  auto const& algo = keyword->pointcuts[ruleStmt.location.pointcut]->aglorithm;
  auto const& pattern = keyword->replacement_patterns[0]; // BUG: only first pattern variant used
  auto const& workingType = keyword->type;

  iFunc->name = ruleId + "_instrummenter_" + ruleStmt.location.pointcut + getUniqueId() + to_string(__LINE__);
  iFunc->copyParamsFrom(currentCallChain.back().get());
  iFunc->searchType = pattern.searchType;
  iFunc->processingType = currentCallChain.back()->processingType;

  // deconstruct current node
  {
    auto& stmt = iFunc->statements.emplace_back(/* empty */);
    stmt.action = "deconstruct " + CURRENT_NODE;

    stringstream ss;
    grammar->types.at(workingType)->variants[0].toTXLWithNames(ss, typeToName); // BUG: only first variant used
    stmt.text = ss.str();
  }

  // ==========

  // introduce common variables and constants
  {
    auto& stmt = iFunc->statements.emplace_back(/* empty */);
    stmt.action = "construct NODE [id]";
    stmt.text = "_ [typeof " + CURRENT_NODE + "]";
  }

  {
    auto& stmt = iFunc->statements.emplace_back(/* empty */);
    stmt.action = "construct POINTCUT [stringlit]";
    stmt.text = "_ [+ \"" + ruleStmt.location.pointcut + "\"]";
  }

  {
    auto& stmt = iFunc->statements.emplace_back(/* empty */);
    stmt.action = "construct FILE [stringlit]";
    stmt.text = "_ [+ \"" + processingFilename + "\"]";
  }

  context;// FIXME: add variables from POI

  // ==========

  unordered_set<string_view> syntheticVariables;
  for (auto const& make : ruleStmt.actionMake) {
    auto& constructStmt = iFunc->statements.emplace_back(/* empty */);
    constructStmt.action = "construct " + make.target + " [stringlit]";

    stringstream ss;
    for (auto const& component : make.components)
      component->toTXL(ss);
    constructStmt.text = "_" + ss.str();

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
          replacement += typeToName(ref->typeId) + ' ';
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
              ::callAlgorithmCommand({
                                       .function = step.function,
                                       .args = step.args,
                                       .preparedFragment = preparedFragment,
                                       .iFunc = iFunc.get()
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

  addToCallChain(std::move(iFunc));
}

void TXLGenerator::genMain(ostream& str)
{
  TXLFunction main;
  main.name = "main";

  auto& replaceStmt = main.statements.emplace_back(/* empty */);
  replaceStmt.action = "replace [program]";
  replaceStmt.text = "Program [program]";

  auto& byStmt = main.statements.emplace_back(/* empty */);
  byStmt.action = "by";
  byStmt.text = "Program\n";

  for (auto const& chain : callTree)
    byStmt.text += "\t\t[" + chain.front()->name + "]\n";

  main.generateTXL(str);
}

void TXLGenerator::genUtilityFunctions(ostream& str)
{
  for (auto const& f : annotation->library)
    f->name;; // FIXME: !!! implement !!!
}

// NOTE: assumption: conjunctions are correct and contain references only to basic contexts
void TXLGenerator::compile()
{
  loadRequestedFragments();
  evaluateKeywordsDistances();

  for (auto const& [_, rule] : ruleset->rules) {
    SCIS_DEBUG("Processing rule \'" << rule->id << "\'");

    for (auto const& ruleStmt : rule->statements) {
      // FIXME: global context
      if (ruleStmt.location.contextId == "@")
        continue;

      auto const context = ruleset->contexts.at(ruleStmt.location.contextId).get();
      SCIS_DEBUG("Processing statement");

      // generate chain of functions
      SCIS_DEBUG("compiling Collectors...");
      compileCollectionFunctions(rule->id, context);

      SCIS_DEBUG("compiling Filter...");
      compileFilteringFunction(rule->id, context);

      SCIS_DEBUG("compiling Path*...");
      compileRefinementFunctions(rule->id, ruleStmt);

      SCIS_DEBUG("compiling Instrumenter...");
      compileInstrumentationFunction(rule->id, ruleStmt, context);

      // add to whole program
      callTree.emplace_back(std::move(currentCallChain));
    }
  }
}

void TXLGenerator::generateCode(ostream& str)
{
  SCIS_DEBUG("Generating TXL sources");

  // generate utility functions

  // generate all chains
  for (auto const& chain : callTree)
    for (auto const& func : chain) {
      str << endl;
      func->generateTXL(str);
      str << endl;
    }

  // generate main function
  genMain(str);

  genUtilityFunctions(str);
}
