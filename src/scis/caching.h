#ifndef CACHING_H
#define CACHING_H

#include "common.h"
#include <ostream>

namespace scis::caching {

  using namespace std;

  struct CacheSignData {
    string lang = "???";
    string stamp = "???";

    string ver = scis::PROGRAM_VERSION;
  };

  string generateFilenameByRuleset(string const& rulesetFilename);

  CacheSignData initCacheSign(string const& rulesetFilename, string const& language);

  void applyCacheFileSign(ostream& stream, CacheSignData const& data);

  bool checkCache(string const& outTxlFilename,
                  CacheSignData const& data);

} // scis

#endif // CACHING_H
