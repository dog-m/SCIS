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

static unordered_map<string_view, string_view> CTX_OP_INVERSION_MAPPING {
  {"=", "~="},
  {"~=", "="},
  {"<", ">="},
  {"<=", ">"},
  {">", "<="},
  {">=", "<"},
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

void TXLGenerator::doStuff()
{
  ; // представим, что конъюнкции составлены верно

  //----

  // построение максимальных растояний до корня (BFS)
  unordered_map<string_view, int> maxDistanceToRoot;

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

  //-----

  struct TXLFunction {
    struct Parameter {
      string id;
      string type;
    };

    struct Statement {
      string action;
      string text;
    };

    bool isRule = false;
    string name = "???";
    vector<Parameter> params;
    vector<Statement> body;
    optional<string> skipType = nullopt;

    virtual ~TXLFunction() = default;

    virtual void prepareBody() {}

    void copyParamsFrom(TXLFunction const *const from) {
      params.insert(params.end(), from->params.cbegin(), from->params.cend());
    }

    void toTXL(ostream &ss) {
      ss << ruleOrFunction() << " " << name;
      for (auto const& p : params)
        ss << ' ' << p.id << " [" << p.type << ']';
      ss << endl;

      if (skipType.has_value())
        ss << "  skipping [" << skipType.value() << ']' << endl;

      prepareBody();
      ss << endl;
      for (auto const& line : body)
        ss << line.action << endl
           << line.text << endl;

      ss << "end " << ruleOrFunction();
    }

    string_view ruleOrFunction() {
      return isRule ? "rule" : "function";
    }
  }; // TXLFunction

  struct CallChain {
    TXLFunction const* callFrom = nullptr; /// вызываЮЩИЙ элемент в цепочке
    TXLFunction const* callTo = nullptr; /// вызываЕМЫЙ элемент
  };

  struct CallChainElement : public TXLFunction, public CallChain {

    void connectTo(CallChainElement *const other) {
      this->callTo = other;
      other->callFrom = this;
    }
  };

  // функция для сбора информации для функции фильтрации
  struct CollectionFunction : public CallChainElement {
    string keyword;
    string keywordType;
  };

  // функция фильтрации
  struct FilteringFunction : public CallChainElement {
    struct Where {
      struct CallElement {
        string name;
        vector<string> args;
      };

      string target;
      vector<CallElement> operators;
    }; // Where

    vector<Where> wheres;
  }; // FilteringFunction

  // уточняющая функция
  struct RefinementFunction : public CallChainElement {
    ;
  };

  // функция, выплняющая инструментирование
  struct InstrumentationFunction : public TXLFunction {
    ;
  };

  //======================================================

  vector<string> addThisToMainFunctionAsListOfFunctionCalls;

  for (auto const& [_, rule] : ruleset->rules) {
    SCIS_DEBUG("Processing rule \'" << rule->id << "\'");

    for (auto const& stmt : rule->statements) {
      // FIXME: global context
      if (stmt.location.contextId == "@")
        continue;
      auto const context = ruleset->contexts.at(stmt.location.contextId).get();
      SCIS_DEBUG("Processing statement");

      if (auto const compoundCtx = dynamic_cast<CompoundContext*>(context)) {
        unordered_set<string_view> keywordsUsedInContext;
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

        // sorting keywords
        vector<string_view> sortedKeywords(keywordsUsedInContext.begin(), keywordsUsedInContext.end());
        std::sort(sortedKeywords.begin(), sortedKeywords.end(), [&](string_view a, string_view b) {
            return maxDistanceToRoot.at(a) < maxDistanceToRoot.at(b);
          });

        SCIS_DEBUG("sorted keywords:");
        for (auto const& k : sortedKeywords)
          cout << k << endl;

        vector<unique_ptr<TXLFunction>> callChain;

        // create and add collection functions to a sequence  ==================================================
        for (auto const& keyword : sortedKeywords) {
          auto cFunc = make_unique<CollectionFunction>();
          cFunc->isRule = true; // BUG: is yellow always a rule?

          if (!callChain.empty()) {
            auto const lastCollector = dynamic_cast<CollectionFunction*>(callChain.back().get());

            // add params of last collection function
            cFunc->copyParamsFrom(lastCollector);

            // plus one parameter (result) from last collection function
            auto& cParam = cFunc->params.emplace_back(/* empty */);
            cParam.id = lastCollector->keyword;
            cParam.type = lastCollector->keywordType;

            // strong chaining
            lastCollector->connectTo(cFunc.get());
            //cFunc->skipType = lastCollector->keyword; // BUG: potential skipping bug
          }

          cFunc->name = rule->id + "_collector_" + context->id + "_" + cFunc->keyword + to_string(__LINE__);
          cFunc->keyword = keyword;
          cFunc->keywordType = annotation->grammar.graph.keywords.at(keyword)->type;
          cFunc->skipType = cFunc->keywordType; // BUG: potential skipping bug

          callChain.emplace_back(std::move(cFunc));
        }

        // add context filtering function ==================================================
        auto fFunc = make_unique<FilteringFunction>();

        fFunc->name = rule->id + "_filter_" + context->id + to_string(__LINE__);
        fFunc->copyParamsFrom(callChain.back().get());

        unordered_map<string, string> fVarName;
        for (auto const& par : fFunc->params)
          fVarName.insert(par.type, par.id);

        // *replace* thing
        auto const fType = sortedKeywords.back().data();
        auto& fReplaceStmt = fFunc->body.emplace_back(/* empty */);
        fReplaceStmt.action = "replace $ ["s + fType + ']';
        fReplaceStmt.text = "_ ["s + fType + "]";

        for (auto const& disjunction : compoundCtx->references)
          for (auto const& element : disjunction) {
            // FIXME: compound -> basic context resolution
            auto const basicCtx = dynamic_cast<BasicContext*>(ruleset->contexts.at(element.id).get());

            // FIXME: only first POI used
            auto const& constraint = basicCtx->constraints.front();

            auto& where = fFunc->wheres.emplace_back(/* empty */);
            auto const poi = annotation->pointsOfInterest[constraint.id].get();

            if (poi->valueTypePath.empty()) {
              auto const& targetType = annotation->grammar.graph.keywords[poi->keyword]->type;
              where.target = fVarName[targetType] + "_str";

              auto& stmt = fFunc->body.emplace_back(/* empty */);
              stmt.action = "construct " + where.target + " [stringlit]";
              stmt.text = "_ [quote " + fVarName[targetType] + "]";
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

                stringstream pattern;
                grammar->types[type]->variants[0].toTXLWithNames(pattern, typeToName);

                auto& stmt = fFunc->body.emplace_back(/* empty */);
                stmt.action = "deconstruct " + fVarName[type];
                stmt.text = pattern.str();
              }

              path.back();

              auto& stmt = fFunc->body.emplace_back(/* empty */);
              stmt.action = "construct " + path.back() + "_str [stringlit]";
              stmt.text = "_ [quote " + path.back() + "]";
            }

            // FIXME: different context constraints for the same node
            auto& call = where.operators.emplace_back(/* empty */);
            call.name = element.negation ? CTX_OP_INVERSION_MAPPING[constraint.op] : constraint.op;
            call.args.push_back(constraint.value.text); // FIXME: patterns
          }

        callChain.emplace_back(std::move(fFunc));

        // add path functions ==================================================
        bool rFirst = true;
        for (auto const& el : stmt.location.path) {
          auto rFunc = make_unique<RefinementFunction>();

          rFunc->copyParamsFrom(callChain.back().get());

          rFunc;
          el;

          dynamic_cast<CallChainElement*>(callChain.back().get())->connectTo(rFunc.get());
          callChain.emplace_back(std::move(rFunc));
        }

        // add instrumentation function ==================================================
        auto iFunc = make_unique<InstrumentationFunction>();
        auto const keyword = annotation->grammar.graph.keywords[stmt.location.path.back().keywordId].get(); // FIXME: empty path?
        auto const& algo = keyword->pointcuts[stmt.location.pointcut]->aglorithm;
        auto const& pattern = keyword->replacement_patterns[0];

        iFunc;
        algo;
        pattern;

        callChain.emplace_back(std::move(iFunc));

        // finally ==================================================
        addThisToMainFunctionAsListOfFunctionCalls.emplace_back(callChain.front()->name);
      }
      else if (auto const basicCtx = dynamic_cast<BasicContext*>(context)) {
        basicCtx;
        ; // FIXME: basic context
      }
      else
        SCIS_ERROR("Unknown context type");
    }
  }
}
