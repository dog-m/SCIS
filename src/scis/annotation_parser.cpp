#include "annotation_parser.h"
#include "logging.h"

using namespace std;
using namespace scis;
using namespace tinyxml2;

#include "../xml_parser_utils.h"

void AnnotationParser::parseGrammar(XMLElement const* const root)
{
  // base params
  annotation->grammar.language = expectedAttribute(root, "language")->Value();
  annotation->grammar.txlSourceFilename = expectedAttribute(root, "src")->Value();

  parseKeywordSubtypes(expectedPath(root, { "keyword-DAG" }));
}

inline void AnnotationParser::parseKeywordSubtypes(const XMLElement* const root)
{
  FOREACH_XML_ELEMENT(root, subtype)
    if (subtype->Attribute("type"))
      parseKeyword(subtype);
}

void AnnotationParser::parseKeyword(XMLElement const* const keyword)
{
  auto word = make_unique<GrammarAnnotation::DirectedAcyclicGraph::Keyword>();

  // base params
  word->id = keyword->Name();
  word->type = expectedAttribute(keyword, "type")->Value();
  FOREACH_XML_ELEMENT(keyword, subtype)
    word->subnodes.push_back(subtype->Name());

  annotation->grammar.graph.keywords.emplace(word->id, std::move(word));

  parseKeywordSubtypes(keyword);
}

void AnnotationParser::parseLibrary(XMLElement const* const root)
{
  FOREACH_XML_ELEMENT(root, func) {
    auto function = make_unique<GrammarAnnotation::Function>();

    function->name = expectedAttribute(func, "name")->Value();

    string_view const policy = expectedAttribute(func, "apply")->Value();
    if (policy == "after-all")
      function->callPolicy = GrammarAnnotation::FunctionPolicy::AFTER_ALL;
    else
      if (policy == "before-all")
        function->callPolicy = GrammarAnnotation::FunctionPolicy::BEFORE_ALL;
    else
      function->callPolicy = GrammarAnnotation::FunctionPolicy::DIRECT_CALL;

    // optional list of parameters
    if (auto const params = func->FindAttribute("params"))
        processList(',', params->Value(), [&](string const& nameWithType){
          auto const pos = nameWithType.find(':');
          if (pos == string::npos)
            throw "Parameter type expected"s;

          auto& param = function->params.emplace_back(/* empty */);

          param.id = nameWithType.substr(0, pos);
          param.type = nameWithType.substr(pos + 1);
        });

    function->source = expectedPath(func, { "source" })->GetText();

    annotation->library.emplace_back(std::move(function));
  }
}

void AnnotationParser::parsePointsOfInterest(XMLElement const* const root)
{
  FOREACH_XML_ELEMENT(root, element) {
    auto poi = make_unique<GrammarAnnotation::PointOfInterest>();

    poi->id = "poi:"s + expectedAttribute(element, "id")->Value();

    poi->keyword = expectedAttribute(element, "keyword")->Value();

    processList(':', expectedAttribute(element, "value-of")->Value(), [&](string const& txlTypeId){
      poi->valueTypePath.emplace_back(txlTypeId);
    });

    annotation->pointsOfInterest.emplace(poi->id, std::move(poi));
  }
}

void AnnotationParser::parsePointcuts(XMLElement const* const root)
{
  FOREACH_XML_ELEMENT(root, keyword)
    parsePointcutsForKeyword(keyword);
}

void AnnotationParser::parsePointcutsForKeyword(XMLElement const* const root)
{
  auto const name = expectedAttribute(root, "name")->Value();
  auto const keyword = annotation->grammar.graph.keywords.at(name).get();

  // parsing patterns for a keyword
  auto const patterns = expectedPath(root, { "replacement-patterns" });
  FOREACH_XML_ELEMENT(patterns, pat) {
    auto& pattern = keyword->replacement_patterns.emplace_back(/* empty */);

    pattern.searchType = expectedAttribute(pat, "search-type")->Value();

    FOREACH_XML_NODE(pat, block) {
      // is it just text?
      if (auto const text = block->ToText()) {
        auto txt = make_unique<GrammarAnnotation::Pattern::TextBlock>();

        txt->text = text->Value();

        pattern.blocks.emplace_back(std::move(txt));
      }
      else
        // or actual element?
        if (auto const elem = block->ToElement()) {
          auto const elementName = elem->Name();
          // pointcut
          if (elementName == "p"sv) {
            auto pointcut = make_unique<GrammarAnnotation::Pattern::PointcutLocation>();

            pointcut->name = expectedAttribute(elem, "name")->Value();

            pattern.blocks.emplace_back(std::move(pointcut));
          }
          else
            throw "Unrecognized element <"s + elementName + ">"s;
        }
      else
        // probably a type-reference comment
        if (auto const comm = block->ToComment()) {
          auto ref = make_unique<GrammarAnnotation::Pattern::TypeReference>();

          ref->typeId = comm->Value();

          pattern.blocks.emplace_back(std::move(ref));
        }
      else
        throw "Unknown element in pointcut"s;
    }
  }

  // parsing actual pointcuts
  FOREACH_XML_ELEMENT(expectedPath(root, { "pointcuts" }), point) {
    auto pointcut = make_unique<GrammarAnnotation::Pointcut>();

    pointcut->name = expectedAttribute(point, "name")->Value();

    auto const ref = expectedPath(point, { "reference" });
    pointcut->refType = expectedAttribute(ref, "type")->Value();
    pointcut->refAlias = expectedAttribute(ref, "as")->Value();

    auto const algo = expectedPath(point, { "paste-algorithm" });
    FOREACH_XML_ELEMENT(algo, action) {
      auto& step = pointcut->aglorithm.emplace_back(/* empty */);

      step.function = action->Name();
      FOREACH_XML_ATTRIBUTE(action, argument)
        step.args.emplace(argument->Name(), argument->Value());
    }

    keyword->pointcuts.emplace(pointcut->name, std::move(pointcut));
  }
}

unique_ptr<GrammarAnnotation> AnnotationParser::parse(XMLDocument const& doc)
{
  annotation.reset(new GrammarAnnotation);

  try {
    parseGrammar(expectedPath(&doc, { "annotation", "grammar" }));

    parseLibrary(expectedPath(&doc, { "annotation", "lib" }));

    parsePointsOfInterest(expectedPath(&doc, { "annotation", "points-of-interest" }));

    parsePointcuts(expectedPath(&doc, { "annotation", "pointcuts" }));

    // skip everything else
  } catch (string const msg) {
    SCIS_ERROR("Incorrect grammar annotation. " << msg);
  }

  return std::move(annotation);
}