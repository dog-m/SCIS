#include "txl_generator.h"
#include "logging.h"

#include <functional>
#include <deque>
#include <unordered_set>
#include <sstream>

using namespace std;
using namespace scis;

static unordered_map<string_view, function<void()>> STANDARD_FUNCTIONS {
{ "insert-reference", [](){
  ;
}},
{ "insert-call", [](){
  ;
}},
};

static string const CURRENT_NODE = "__NODE__";

static unordered_map<string_view, string_view> CTX_OP_INVERSION_MAPPING {
  { "=", "~=" },
  { "~=", "=" },
  { "<", ">=" },
  { "<=", ">" },
  { ">", "<=" },
  { ">=", "<" },
};


static auto getUniqueId()
{
  static uint64_t id = 0;
  return "uid" + to_string(id++);
}

static string typeToName(string_view const& typeName) {
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


void TXLGenerator::TXLFunction::copyParamsFrom(const TXLGenerator::TXLFunction* const from)
{
  params.insert(params.end(), from->params.cbegin(), from->params.cend());
}

void TXLGenerator::TXLFunction::generateTXL(ostream& ss)
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

string_view TXLGenerator::TXLFunction::ruleOrFunction()
{
  return isRule ? "rule" : "function";
}

void TXLGenerator::CallChainFunction::connectTo(TXLGenerator::CallChainFunction* const other)
{
  this->callTo = other;
  other->callFrom = this;
}

void TXLGenerator::CollectionFunction::generateStatements()
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

void TXLGenerator::FilteringFunction::generateStatements()
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

void TXLGenerator::RefinementFunction::generateStatements()
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

void TXLGenerator::InstrumentationFunction::generateStatements()
{
  ;
}

Fragment const* TXLGenerator::getFragment(const string_view& name)
{
  auto const frag = fragLibrary.find(name);
  if (frag != fragLibrary.cend())
    return frag->second.get();

  else {
    static FragmentParser parser;

    auto f = parser.parse(name);
    auto const fragment = f.get();
    fragLibrary.emplace(f->name, std::move(f));

    return fragment;
  }
}

void TXLGenerator::evaluateDistances()
{
  maxDistanceToRoot.clear();

  deque<pair<string_view, int>> queue;
  for (auto const key : annotation->grammar.graph.topKeywords) {
    queue.push_back(make_pair(key->id, 0));

    // make first pass over top-most keywords work
    maxDistanceToRoot.emplace(key->id, -1);
  }

  constexpr auto maxIter = 2500;
  auto iter = 0;
  while (!queue.empty() && iter++ < maxIter) {
    auto const [candidate, newDistance] = std::move(queue.front());
    queue.pop_front();

    auto& oldDistance = maxDistanceToRoot[candidate];
    if (oldDistance < newDistance) {
      oldDistance = newDistance;

      for (auto const& node : annotation->grammar.graph.keywords[candidate]->subnodes)
        queue.push_back(make_pair(node, newDistance + 1));
    }
  }
  if (iter > maxIter)
    SCIS_ERROR("Cycle detected in grammar annotation (see keyword-DAG)");
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

        // FIXME: only first POI used
        auto const& constraint = basicCtx->constraints.front();
        auto const& poi = annotation->pointsOfInterest.at(constraint.id).get();

        keywordsUsedInContext.insert(poi->keyword);
      }
  else if (auto const basicCtx = dynamic_cast<BasicContext const*>(context)) {
    // FIXME: only first POI used
    auto const& constraint = basicCtx->constraints.front();
    auto const& poi = annotation->pointsOfInterest.at(constraint.id).get();

    keywordsUsedInContext.insert(poi->keyword);
  }
  else
    SCIS_ERROR("Unknown context type");

  // sorting keywords
  vector<string_view> sortedKeywords(keywordsUsedInContext.begin(), keywordsUsedInContext.end());
  std::sort(sortedKeywords.begin(), sortedKeywords.end(), [&](string_view a, string_view b) {
      return maxDistanceToRoot.at(a) < maxDistanceToRoot.at(b);
    });

  SCIS_DEBUG("sorted keywords:");
  for (auto const& k : sortedKeywords)
    cout << k << endl;

  // create and add collection functions to a sequence
  for (auto const& keyword : sortedKeywords) {
    auto cFunc = make_unique<CollectionFunction>();
    cFunc->isRule = true; // BUG: is yellow always a rule?

    if (!currentCallChain.empty()) {
      auto const lastCollector = dynamic_cast<CollectionFunction*>(currentCallChain.back().get());

      // add params of last collection function
      cFunc->copyParamsFrom(lastCollector);

      // plus one parameter (result) from last collection function
      auto& cParam = cFunc->params.emplace_back(/* empty */);
      cParam.id = lastCollector->processingKeyword;
      cParam.type = lastCollector->processingType;

      // strong chaining
      lastCollector->connectTo(cFunc.get());
      //cFunc->skipType = lastCollector->keyword; // BUG: potential skipping bug
    }

    cFunc->name = ruleId + "_collector_" + context->id + "_" + cFunc->processingKeyword + to_string(__LINE__);
    cFunc->processingKeyword = keyword;
    cFunc->processingType = annotation->grammar.graph.keywords.at(keyword)->type;
    cFunc->skipType = cFunc->processingType; // BUG: potential skipping bug

    currentCallChain.emplace_back(std::move(cFunc));
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
    fVarName.insert(par.type, par.id);

  // *replace* thing
  fFunc->processingType = annotation->grammar.graph.keywords.at(lastCollector->processingType)->type;

  if (auto const compoundCtx = dynamic_cast<CompoundContext const*>(context))
    for (auto const& disjunction : compoundCtx->references)
      for (auto const& element : disjunction) {
        // FIXME: compound -> basic context resolution
        auto const basicCtx = dynamic_cast<BasicContext*>(ruleset->contexts.at(element.id).get());

        // FIXME: only first POI used
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
                // FIXME: variable name duplication
              });

            deconstructStmt.text = pattern.str();
          }

          where.target = fVarName[path.back()] + "_str";

          auto& constructStmt = fFunc->statements.emplace_back(/* empty */);
          constructStmt.action = "construct " + path.back() + " [stringlit]";
          constructStmt.text = "_ [quote " + fVarName[path.back()] + "]";
        }

        // FIXME: different context constraints for the same node
        auto& call = where.operators.emplace_back(/* empty */);
        call.name = element.negation ? CTX_OP_INVERSION_MAPPING[constraint.op] : constraint.op;
        call.args.push_back(constraint.value.text); // FIXME: patterns
      }
  else
    SCIS_ERROR("Filtering for basic contexts not implemented");// FIXME: filter basic contexts

  currentCallChain.emplace_back(std::move(fFunc));
}

void TXLGenerator::compileRefinementFunctions(string const& ruleId,
                                              Rule::Statement const& ruleStmt)
{
  // add path functions
  for (auto const& el : ruleStmt.location.path) {
    auto rFunc = make_unique<RefinementFunction>();

    rFunc->name = ruleId + "_filter_" + el.keywordId + getUniqueId() + to_string(__LINE__);
    rFunc->copyParamsFrom(currentCallChain.back().get());

    rFunc->skipType = annotation->grammar.graph.keywords[el.keywordId]->type;
    rFunc->processingType = rFunc->skipType.value();

    dynamic_cast<CallChainFunction*>(currentCallChain.back().get())->connectTo(rFunc.get());
    currentCallChain.emplace_back(std::move(rFunc));
  }
}

void TXLGenerator::compileInstrumentationFunction(string const& ruleId,
                                                  Rule::Statement const& ruleStmt,
                                                  Context const* const context)
{
  // add instrumentation function
  auto iFunc = make_unique<InstrumentationFunction>();
  auto const keyword = annotation->grammar.graph.keywords[ruleStmt.location.path.back().keywordId].get(); // FIXME: empty path?
  auto const& algo = keyword->pointcuts[ruleStmt.location.pointcut]->aglorithm;
  auto const& pattern = keyword->replacement_patterns[0];

  iFunc->name = ruleId + "_instrummenter_" + ruleStmt.location.pointcut + getUniqueId() + to_string(__LINE__);
  iFunc->copyParamsFrom(currentCallChain.back().get());

  iFunc;
  ruleStmt.location.pointcut;

  context;
  ruleStmt.actionMake;

  algo;
  pattern;
  ruleStmt.actionAdd;

  currentCallChain.emplace_back(std::move(iFunc));
}

void TXLGenerator::genMain(ostream& str)
{
  TXLFunction main;
  main;

  callTree;
}

void TXLGenerator::compile()
{
  ; // представим, что конъюнкции составлены верно

  evaluateDistances();

  for (auto const& [_, rule] : ruleset->rules) {
    SCIS_DEBUG("Processing rule \'" << rule->id << "\'");

    for (auto const& ruleStmt : rule->statements) {
      // FIXME: global context
      if (ruleStmt.location.contextId == "@")
        continue;

      auto const context = ruleset->contexts.at(ruleStmt.location.contextId).get();
      SCIS_DEBUG("Processing statement");

      // generate chain of functions
      SCIS_DEBUG("generating Collectors...");
      compileCollectionFunctions(rule->id, context);

      SCIS_DEBUG("generating Filter...");
      compileFilteringFunction(rule->id, context);

      SCIS_DEBUG("generating Path*...");
      compileRefinementFunctions(rule->id, ruleStmt);

      SCIS_DEBUG("generating Instrumenter...");
      compileInstrumentationFunction(rule->id, ruleStmt, context);

      // add to whole program
      callTree.emplace_back(std::move(currentCallChain));
    }
  }
}

void TXLGenerator::generateCode(ostream& str)
{
  str << __PRETTY_FUNCTION__ << endl;
}
