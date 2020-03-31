#ifndef ANNOTATION_PARSER_H
#define ANNOTATION_PARSER_H

#include <tinyxml2/tinyxml2.h>
#include "annotation.h"

namespace scis {

  using namespace std;
  using namespace tinyxml2;

  class AnnotationParser final {
    unique_ptr<GrammarAnnotation> annotation;

    void parseBaseParameters(XMLElement const* const root);

    void parseGrammar(XMLElement const* const root);

    inline void parseKeywordSubtypes(XMLElement const* const root);

    void parseKeyword(XMLElement const* const keyword);

    void parseLibrary(XMLElement const* const root);

    void parsePointsOfInterest(XMLElement const* const root);

    void parsePointcuts(XMLElement const* const root);

    void parsePointcutsForKeyword(XMLElement const* const root);

  public:
    unique_ptr<GrammarAnnotation> parse(XMLDocument const& doc);
  };

} // scis

#endif // ANNOTATION_PARSER_H
