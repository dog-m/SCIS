#ifndef TXL_GENERATOR_H
#define TXL_GENERATOR_H

#include "../txl/grammar.h"
#include "annotation.h"
#include "fragment.h"
#include "ruleset.h"

#include "fragment_parser.h"
#include "txl_generator_commons.h"

namespace scis {

  using namespace std;
  using namespace scis::codegen;

  /// one-time use
  class TXLGenerator final {
    using RefinementFunctionGenerator = function<void(string const&, Rule::Location::PathElement const&, int const)>;

  public:
    unique_ptr<txl::Grammar> grammar;

    GrammarAnnotation* annotation;

    unique_ptr<Ruleset> ruleset;

    string fragmentsDir;

  protected:
    unordered_map<string, unique_ptr<Fragment>> loadedFragments;

    unordered_map<string, int> maxDistanceToRoot;

    vector<CallChainFunction*> currentCallChain;

    vector<unique_ptr<TXLFunction>> functions;

    deque<TXLFunction const*> mainCallSequence;

    unordered_map<GrammarAnnotation::PointOfInterest const*, TXLFunction const*> poi2getter;

    unordered_map<string, ContextChecker const*> contextCheckers;

    unordered_map<string, TXLFunction const*> operator2wrapper;

    int maxRefinementIndex = 0;

  protected:
    Fragment const* getFragment(string const& id);

    void addToCallChain(CallChainFunction *const func);

    void resetCallChain();

    CallChainFunction* lastCallChainElement();

    void wrapStandardBinnaryOperator(string const& op,
                                     string const& type,
                                     string const& name,
                                     string const& internalFunction);

    string getWrapperForOperator(string const& op,
                                 string const& type);

    void evaluateKeywordsDistances();

    void loadRequestedFragments();

    template <typename Kind>
    Kind* createFunction();

    void sortKeywords(vector<string>& keywords) const;

    void compilePOIGetters();

    void compileGetterForPOI(GrammarAnnotation::PointOfInterest const* const poi);

    void compileContextCheckers();

    ContextChecker* prepareNewContextChecker(Context const* const context,
                                             bool const positive);

    void registerNewContextChecker(Context const* const context,
                                   ContextChecker const* const checker);

    ContextChecker const* findContextCheckerByContext(string const& name);

    void unrollPatternFor(TXLFunction* const rFunc,
                          string const& keywordId,
                          Pattern const& pattern,
                          string const& variableName);

  protected:
    void compileBasicContext(BasicContext const* const context);

    void compileContextNegation(Context const* const context,
                                ContextChecker const* const contextChecker);

    void compileCompoundContext(CompoundContext const* const context);

    void compileCollectionFunctions(string const& ruleId,
                                    Context const* const context);

    void compileFilteringFunction(string const& ruleId,
                                  Context const* const context);

    void compileRefinementFunctions(string const& ruleId,
                                    Rule::Statement const& ruleStmt);

    void compileRefinementFunctionStarter();

    void compileRefinementFunction_First(string const& name,
                                         Rule::Location::PathElement const& element,
                                         int const index);

    void compileRefinementFunction_All(string const& name,
                                       Rule::Location::PathElement const& element,
                                       int const index);

    void compileRefinementFunction_Level(string const& name,
                                         Rule::Location::PathElement const& element,
                                         int const index);

    void compileRefinementFunction_LevelPredicate(string const& name,
                                                  Rule::Location::PathElement const& element,
                                                  int const index);

    void compileRules();

    void compileRefinementHelperFunctions();

    static string getSkipNodeName(int const index);

    static string getSkipDecrementerName(int const index);

    static string getSkipCounterName(string const& refiner);

    void compileInstrumentationFunction(string const& ruleId,
                                        Rule::Statement const& ruleStmt);

    string prepareFragment(Fragment const* const fragment,
                           vector<string> const& args);

    void compileMain();

    void compileAnnotationUtilityFunctions();

    void compileStandardWrappers(string const& baseType);

    void includeGrammar(ostream &str);

  public:
    void compile();

    void generateCode(ostream &str);
  }; // TXLGenerator

} // scis

#endif // TXL_GENERATOR_H
