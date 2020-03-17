#ifndef TXL_GENERATOR_H
#define TXL_GENERATOR_H

#include "../txl/grammar.h"
#include "annotation.h"
#include "fragment.h"
#include "ruleset.h"

#include "fragment_parser.h"
#include <deque>

namespace scis {

  using namespace std;

  struct TXLGenerator {

    // sub-types

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
      string processingType = "???";
      deque<Statement> statements;
      optional<string> skipType = nullopt;

      virtual ~TXLFunction() = default;

      virtual void generateStatements() {}

      void copyParamsFrom(TXLFunction const *const from);

      void generateTXL(ostream &ss);

      string_view ruleOrFunction();
    }; // TXLFunction

    struct CallChainElement {
      /// вызываЮЩИЙ элемент в цепочке
      TXLFunction const* callFrom = nullptr;
      /// вызываЕМЫЙ элемент
      TXLFunction const* callTo = nullptr;
    };

    struct CallChainFunction : public TXLFunction, public CallChainElement {

      void connectTo(CallChainFunction *const other);
    };

    // функция для сбора информации для функции фильтрации
    struct CollectionFunction : public CallChainFunction {
      string processingKeyword;

      void generateStatements() override;
    };

    // функция фильтрации
    struct FilteringFunction : public CallChainFunction {
      struct Where {
        struct CallElement {
          string name;
          vector<string> args;
        };

        string target;
        vector<CallElement> operators;
      }; // Where

      vector<Where> wheres;

      void generateStatements() override;
    }; // FilteringFunction

    // уточняющая функция
    struct RefinementFunction : public CallChainFunction {
      void generateStatements() override;
    };

    // функция, выплняющая инструментирование
    struct InstrumentationFunction : public TXLFunction {
      ;

      void generateStatements() override;
    };

    // private data

    unique_ptr<txl::Grammar> grammar;
    unique_ptr<GrammarAnnotation> annotation;
    unique_ptr<Ruleset> ruleset;

    unordered_map<string_view, unique_ptr<Fragment>> fragLibrary;

    unordered_map<string_view, int> maxDistanceToRoot;

    using CallChain = vector<unique_ptr<TXLFunction>>;
    CallChain currentCallChain;
    vector<CallChain> callTree;

    Fragment const* getFragment(string_view const& name);

    /// построение максимальных растояний до корня (BFS)
    void evaluateDistances();

    void compileCollectionFunctions(string const& ruleId,
                                    Context const* const context);

    void compileFilteringFunction(string const& ruleId,
                                  Context const* const context);

    void compileRefinementFunctions(string const& ruleId,
                                    Rule::Statement const& ruleStmt);

    void compileInstrumentationFunction(string const& ruleId,
                                        Rule::Statement const& ruleStmt,
                                        Context const* const context);

    void genMain(ostream& str);

  public:
    void compile();

    void generateCode(ostream &str);
  }; // TXLGenerator

} // scis

#endif // TXL_GENERATOR_H
