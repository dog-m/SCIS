#ifndef CACHING_H
#define CACHING_H

#include <string>

namespace scis::caching {

  using namespace std;

  string generateFilenameByRuleset(string const& rulesetFilename);

  bool checkCache(string const& outTxlFilename);

} // scis

#endif // CACHING_H
