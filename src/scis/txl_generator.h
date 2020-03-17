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
      /// calleR
      TXLFunction const* callFrom = nullptr;
      /// calleE
      TXLFunction const* callTo = nullptr;
    };

    struct CallChainFunction : public TXLFunction, public CallChainElement {
      void connectTo(CallChainFunction *const other);
    };

    /// Used by filtering function
    struct CollectionFunction : public CallChainFunction {
      string processingKeyword;

      void generateStatements() override;
    };

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

    struct RefinementFunction : public CallChainFunction {
      void generateStatements() override;
    };

    struct InstrumentationFunction : public CallChainFunction {
      void generateStatements() override;
    };

    // private data

    unique_ptr<txl::Grammar> grammar;
    unique_ptr<GrammarAnnotation> annotation;
    unique_ptr<Ruleset> ruleset;
    string processingFilename = "???/???.???"; // FIXME: add file name
    string fragmentsDir = "./";

    unordered_map<string_view, unique_ptr<Fragment>> fragments;

    unordered_map<string_view, int> maxDistanceToRoot;

    using CallChain = vector<unique_ptr<CallChainFunction>>;

    CallChain currentCallChain;
    vector<CallChain> callTree;

    Fragment const* getFragment(string_view const& id);

    void addToCallChain(unique_ptr<CallChainFunction>&& func);

    /// построение максимальных растояний до корня (BFS)
    void evaluateKeywordsDistances();

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
