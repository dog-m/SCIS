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
  //using namespace scis::generation;

  class TXLGenerator {
  public:
    unique_ptr<txl::Grammar> grammar;
    unique_ptr<GrammarAnnotation> annotation;
    unique_ptr<Ruleset> ruleset;
    string processingFilename = "???.???"; // FIXME: add file name
    string fragmentsDir = "./";

  protected:
    unordered_map<string_view, unique_ptr<Fragment>> fragments;

    unordered_map<string_view, int> maxDistanceToRoot;

    using CallChain = vector<unique_ptr<CallChainFunction>>;

    CallChain currentCallChain;
    vector<CallChain> callTree;

    Fragment const* getFragment(string_view const& id);

    void addToCallChain(unique_ptr<CallChainFunction>&& func);

    void evaluateKeywordsDistances();

    void loadRequestedFragments();

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

    void genMain(ostream& str);

    void genUtilityFunctions(ostream &str);

  public:
    void compile();

    void generateCode(ostream &str);
  }; // TXLGenerator

} // scis

#endif // TXL_GENERATOR_H
