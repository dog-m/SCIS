#ifndef TXL_GENERATOR_COMMONS_H
#define TXL_GENERATOR_COMMONS_H

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <ostream>

namespace scis::codegen {

  using namespace std;

  inline string const TXL_TYPE_STRING = "stringlit";
  inline string const TXL_TYPE_ID = "id";
  inline string const TXL_TYPE_NUMBER = "number";

  inline string const CURRENT_NODE = "__NODE__";

  inline string const VOID_NODE = "__VOID__";
  inline string const VOID_NODE_TYPE = "any";
  inline string const VOID_NODE_VALUE = "% void";

  inline string const POI_GETTER_PREFIX = "__POI_get___";

  inline string const CONTEXT_FUNCTION_PREFIX = "__belongs_to_context___";

  inline string const CONTEXT_FUNCTION_NEGATIVE_PREFIX = "__not" + CONTEXT_FUNCTION_PREFIX;

  inline string const VAR_KEYWORD_PREFIX = "kw_";

  inline string const STD_WRAPPER_PREFIX = "__std__";

  string getUniqueId();

  string makeNameFromType(string_view const& typeName);

  string makeNameFromKeyword(string_view const& keywordName);

  string makeNameFromPOIName(string const& poi);

  string makeFunctionNameFromPOIName(string const& poi);

  string makeFunctionNameFromContextName(string const& context,
                                         bool const negative);

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

    /// statement will be added ON TOP of other statements (in source)
    Statement& addStatementTop(string const& action,
                               string const& text);

    /// statement will be added BELOW other statements (in source)
    Statement& addStatementBott(string const& action,
                                string const& text = "");

    Statement& createVariable(string const& name,
                              string const& type,
                              string const& value);

    Parameter& addParameter(string const& name,
                            string const& type);

    Statement& deconstructVariable(string const& name,
                                   string const& pattern);

    Statement& importVariable(string const& name,
                              string const& type);

    Statement& exportVariableCreate(string const& name,
                                    string const& type,
                                    string const& newValue);

    Statement& exportVariableUpdate(string const& name,
                                    string const& newValue);
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
