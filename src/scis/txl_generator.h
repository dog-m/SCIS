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

  class TXLGenerator {
  public:
    unique_ptr<txl::Grammar> grammar;

    unique_ptr<GrammarAnnotation> annotation;

    unique_ptr<Ruleset> ruleset;

    string processingFilename;

    string fragmentsDir;

  protected:
    unordered_map<string_view, unique_ptr<Fragment>> fragments;

    unordered_map<string_view, int> maxDistanceToRoot;

    vector<CallChainFunction*> currentCallChain;

    vector<unique_ptr<TXLFunction>> functions;
    TXLFunction* mainFunction = nullptr;

    deque<TXLFunction const*> addToMain;

    unordered_map<GrammarAnnotation::PointOfInterest const*, TXLFunction const*> poi2getter;

    unordered_map<string_view, TXLFunction const*> contextCheckers;

  protected:
    Fragment const* getFragment(string_view const& id);

    void addToCallChain(CallChainFunction *const func);

    void evaluateKeywordsDistances();

    void loadRequestedFragments();

    template <typename Kind>
    Kind* createFunction();

    void sortKeywords(vector<string_view> &keywords) const;

    void compileContextCheckers();

    void compileGetterForPOI(GrammarAnnotation::PointOfInterest const* const poi);

    TXLFunction* prepareContextChecker(Context const* const context);

    void registerContextChecker(Context const* const context,
                                TXLFunction const* const checker);

    TXLFunction const* findContextCheckerByContext(string const& name);

    void compileBasicContext(BasicContext const* const context);

    bool compileCompoundContext(CompoundContext const* const context);

    void compileCollectionFunctions(string const& ruleId,
                                    Context const* const context);

    void compileFilteringFunction(string const& ruleId,
                                  Context const* const context);

    void compileBasicContext(BasicContext const* const context,
                             bool const topLevelNegation,
                             FilteringFunction* const fFunc,
                             unordered_map<string, string>& type2name);

    void compileRefinementFunctions(string const& ruleId,
                                    Rule::Statement const& ruleStmt);

    void compileInstrumentationFunction(string const& ruleId,
                                        Rule::Statement const& ruleStmt,
                                        Context const* const context);

    void compileMain();

    void compileUtilityFunctions();

    void genTXLImports(ostream &str);

  public:
    void compile();

    void generateCode(ostream &str);
  }; // TXLGenerator

} // scis

#endif // TXL_GENERATOR_H
