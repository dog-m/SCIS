#ifndef TXL_GENERATOR_COMMONS_H
#define TXL_GENERATOR_COMMONS_H

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <ostream>

namespace scis {

  using namespace std;

  inline string const CURRENT_NODE = "__NODE__";
  inline string const VOID_NODE = "__VOID__";

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
    string searchType;
    string replacement;

    void generateStatements() override;
  };

} // scis

#endif // TXL_GENERATOR_COMMONS_H
