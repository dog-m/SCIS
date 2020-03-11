#ifndef FRAGMENT_PARSER_H
#define FRAGMENT_PARSER_H

#include <tinyxml2/tinyxml2.h>
#include "fragment.h"

namespace scis {

  using namespace std;
  using namespace tinyxml2;

  class FragmentParser {
    unique_ptr<scis::Fragment> fragment;

    void parseDependencies(XMLElement const* const dependencies);

    void parseCode(XMLElement const* const code);

    void parseFragment(XMLElement const* const root);

  public:
    unique_ptr<scis::Fragment> parse(XMLDocument const& doc);
  };

} // TXL

#endif // FRAGMENT_PARSER_H
