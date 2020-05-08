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
  inline string const TXL_TYPE_ID     = "id";
  inline string const TXL_TYPE_NUMBER = "number";

  inline string const NODE_TXL_INPUT = "TXLinput";

  inline string const NODE_ANONYMOUS     = "_";
  inline string const NODE_CURRENT       = "__NODE__";
  inline string const NODE_SEQ_TAIL      = "__TAIL__";
  inline string const NODE_SEQ_SINGLE    = "__SINGLE_BOX_ARRAY__";
  inline string const NODE_SEQ_PROCESSED = "__PROCESSED__";

  inline string const NODE_VOID       = "__VOID__";
  inline string const NODE_VOID_TYPE  = "any";
  inline string const NODE_VOID_VALUE = "% void";

  inline string const PREFIX_UNIQUE_ID            = "_uid";
  inline string const PREFIX_POI_GETTER           = "__POI_get___";
  inline string const PREFIX_CONTEXT_FUNCTION     = "__belongs_to_context___";
  inline string const PREFIX_CONTEXT_FUNCTION_NEG = "__not" + PREFIX_CONTEXT_FUNCTION;
  inline string const PREFIX_VAR_KEYWORD          = "kw_";
  inline string const PREFIX_STD                  = "__std__";
  inline string const PREFIX_NODE_SKIP            = "__SKIP__";

  inline string const SUFFIX_HELPER = "__helper";

  inline string const ACTION_NOTHING = PREFIX_STD + "do_nothing";

  string getUniqueId();

  string makeNameFromType(string_view const& typeName);

  string makeNameFromKeyword(string_view const& keywordName);

  string makeNameFromPOIName(string const& poi);

  string makeFunctionNameFromPOIName(string const& poi);

  string makeFunctionNameFromContextName(string const& context,
                                         bool const negative);

  struct TXLFunction {
    struct Parameter final {
      string id;
      string type;
    }; // Parameter

    struct Statement final {
      string action;
      string text;
    }; // Statement

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

    string getRepeatModifier();

    string getParamNames();
  }; // TXLFunction

  struct CallChainElement {
    /// calleR
    TXLFunction const* callFrom = nullptr;
    /// calleE
    TXLFunction const* callTo = nullptr;
  }; // CallChainElement

  struct CallChainFunction : public TXLFunction, public CallChainElement {
    void connectTo(CallChainFunction *const other);
  }; // CallChainFunction

  /// Used by filtering function. Current node will be added at the end
  struct CollectionFunction final : public CallChainFunction {
    string processingKeyword;

    void generateStatements() override;
  }; // CollectionFunction

  struct FilteringFunction final : public CallChainFunction {
    void generateStatements() override;
  }; // FilteringFunction

  struct RefinementFunction : public CallChainFunction {
    string searchType;
    bool sequential = false;
    int queueIndex = 0;
  }; // RefinementFunction

  /// primary used by "level" kind/modification of refiner
  struct RefinementFunctionFilter : public CallChainFunction {
    string searchType;

    void generateStatements() override;
  }; // RefinementFunctionFilter

  struct RefinementFunction_First final : public RefinementFunction {
    void generateStatements() override;
  }; // RefinementFunction_First

  struct RefinementFunction_All final : public RefinementFunction {
    string skipCount;
    string skipCountDecrementer;

    void generateStatements() override;
  }; // RefinementFunction_All

  struct RefinementFunction_Level final : public RefinementFunction {
    string skipCount;
    string skipCountCounter;
    string skipCountDecrementer;

    void generateStatements() override;
  }; // RefinementFunction_Level

  struct InstrumentationFunction final : public CallChainFunction {
    string searchType;
    string replacement;

    void generateStatements() override;
  }; // InstrumentationFunction

} // scis

#endif // TXL_GENERATOR_COMMONS_H
