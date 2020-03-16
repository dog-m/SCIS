#ifndef TXL_GENERATOR_H
#define TXL_GENERATOR_H

#include "../txl/grammar.h"
#include "annotation.h"
#include "fragment.h"
#include "ruleset.h"

#include "fragment_parser.h"

namespace scis {

  using namespace std;

  struct TXLGenerator {
    unique_ptr<txl::Grammar> grammar;
    unique_ptr<GrammarAnnotation> annotation;
    unique_ptr<Ruleset> ruleset;

    unordered_map<string_view, unique_ptr<Fragment>> fragLibrary;

    Fragment const* getFragment(string_view const& name);

    void doStuff();
  };

} // scis

#endif // TXL_GENERATOR_H
