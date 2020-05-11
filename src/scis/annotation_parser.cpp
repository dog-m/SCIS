#include "annotation_parser.h"
#include "logging.h"

using namespace std;
using namespace scis;
using namespace tinyxml2;

#include "../xml_parser_utils.h"

constexpr string_view CLONNING_PATH_SEPARATOR = "::";

void AnnotationParser::parseBaseParameters(XMLElement const* const root)
{
  if(auto const pipeline = root->FindAttribute("pipeline"))
    annotation->pipeline = pipeline->Value();
}

void AnnotationParser::parseGrammar(XMLElement const* const root)
{
  // base params
  annotation->grammar.language = expectedAttribute(root, "language")->Value();
  annotation->grammar.txlSourceFilename = expectedAttribute(root, "src")->Value();

  if (auto const x = root->FindAttribute("user-variable-type"))
    annotation->grammar.userVariableType = x->Value();

  FOREACH_XML_ELEMENT(expectedPath(root, { "keyword-DAG" }), subtype)
    if (subtype->Attribute("type")) {
      parseDAGKeyword(subtype);

      // fill tops
      auto const newKeyword = subtype->Name();
      auto const newTopKeyword = annotation->grammar.graph.keywords.at(newKeyword).get();
      annotation->grammar.graph.topKeywords.push_back(newTopKeyword);
    }
}

inline void AnnotationParser::parseDAGKeywordSubtypes(const XMLElement* const root)
{
  FOREACH_XML_ELEMENT(root, subtype)
    if (subtype->Attribute("type"))
      parseDAGKeyword(subtype);
}

void AnnotationParser::parseDAGKeyword(XMLElement const* const keyword)
{
  auto word = make_unique<GrammarAnnotation::DirectedAcyclicGraph::Keyword>();

  // base params
  word->id = keyword->Name();
  word->type = expectedAttribute(keyword, "type")->Value();
  FOREACH_XML_ELEMENT(keyword, subtype)
    word->subnodes.push_back(subtype->Name());

  annotation->grammar.graph.keywords.insert_or_assign(word->id, std::move(word));

  parseDAGKeywordSubtypes(keyword);
}

void AnnotationParser::parseLibrary(XMLElement const* const root)
{
  FOREACH_XML_ELEMENT(root, func) {
    auto function = make_unique<GrammarAnnotation::Function>();

    function->name = expectedAttribute(func, "name")->Value();
    function->isRule = func->Name() == "rule"sv;

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

    poi->id = ANNOTATION_POI_GROUP_PREFIX + expectedAttribute(element, "id")->Value();

    poi->keyword = expectedAttribute(element, "keyword")->Value();

    processList(':', expectedAttribute(element, "value-of")->Value(), [&](string const& txlTypeId){
      poi->valueTypePath.emplace_back(txlTypeId);
    });

    annotation->pointsOfInterest.insert_or_assign(poi->id, std::move(poi));
  }
}

void AnnotationParser::parsePointcuts(XMLElement const* const root)
{
  FOREACH_XML_ELEMENT(root, keyword)
    parseKeywordWithPointcuts(keyword);
}

void AnnotationParser::parseKeywordWithPointcuts(XMLElement const* const root)
{
  auto const name = expectedAttribute(root, "name")->Value();
  auto const keyword = annotation->grammar.graph.keywords.at(name).get();

  // base information
  keyword->searchType = expectedAttribute(root, "search-type")->Value();

  if (auto const seq = root->FindAttribute("sequential"))
    keyword->sequential = seq->BoolValue();
  else
    keyword->sequential = true;

  if (auto const fPOI = root->FindAttribute("filter-poi"))
    keyword->filterPOI = fPOI->Value();

  // parsing patterns for a keyword
  auto const templates = expectedPath(root, { "templates" });
  FOREACH_XML_ELEMENT(templates, tmpl) {
    // create and attach new template
    auto const newTemplate = keyword->addTemplate(expectedAttribute(tmpl, "kind")->Value());

    if (auto autogen = tmpl->FindAttribute("auto-gen")) {
      // NOTE: only "grammar" auto-generated type supported in a template
      string const generator = autogen->Value();
      if (generator == "grammar")
        newTemplate->generateFromGrammar = true;
      else
        throw "Unsupported template auto-generator <" + generator + ">";
    }

    FOREACH_XML_NODE(tmpl, block) {
      // is it just text?
      if (auto const text = block->ToText()) {
        auto txt = make_unique<GrammarAnnotation::Template::TextBlock>();

        txt->text = text->Value();

        newTemplate->blocks.emplace_back(std::move(txt));
      }
      else
        // or actual element?
        if (auto const elem = block->ToElement()) {
          auto const elementName = elem->Name();
          // pointcut
          if (elementName == "p"sv) {
            auto pointcut = make_unique<GrammarAnnotation::Template::PointcutLocation>();

            pointcut->name = expectedAttribute(elem, "name")->Value();

            newTemplate->blocks.emplace_back(std::move(pointcut));
          }
          else
            throw "Unrecognized element <"s + elementName + ">"s;
        }
      else
        // probably a type-reference comment
        if (auto const comm = block->ToComment()) {
          auto ref = make_unique<GrammarAnnotation::Template::TypeReference>();

          ref->typeId = comm->Value();

          newTemplate->blocks.emplace_back(std::move(ref));
        }
      else
        throw "Unknown element in pointcut"s;
    }
  }

  // parsing actual pointcuts
  FOREACH_XML_ELEMENT(expectedPath(root, { "pointcuts" }), point) {
    // create and attach new pointcut
    auto const pointcut = keyword->addPointcut(expectedAttribute(point, "name")->Value());

    if (auto const clone = point->FindAttribute("clone")) {
      // copy from another pointcut
      string const path = clone->Value();

      auto const sep = path.find(CLONNING_PATH_SEPARATOR);
      if (sep == string::npos)
        throw "Invalid path to source pointcut for " + pointcut->name;

      auto const srcKeyword = path.substr(0, sep);
      auto const srcName = path.substr(sep + CLONNING_PATH_SEPARATOR.size());

      // FIXME: deffer pointcut clonning
      try {
        auto const src = annotation->grammar.graph.keywords.at(srcKeyword)->pointcuts.at(srcName).get();
        // BUG: potential bug in algorithm clonning
        pointcut->aglorithm.insert(pointcut->aglorithm.begin(), src->aglorithm.cbegin(), src->aglorithm.cend());
      } catch (...) {
        throw "Original pointcut <"s + path.data() + "> "
              "for <" + pointcut->name + "> has not been created";
      }
    }
    else {
      // original pointcut
      auto const algo = expectedPath(point, { "paste-algorithm" });
      FOREACH_XML_ELEMENT(algo, action) {
        auto& step = pointcut->aglorithm.emplace_back(/* empty */);

        step.function = action->Name();
        FOREACH_XML_ATTRIBUTE(action, argument)
          step.args.insert_or_assign(argument->Name(), argument->Value());
      }
    }
  }
}

unique_ptr<GrammarAnnotation> AnnotationParser::parse(XMLDocument const& doc)
{
  annotation.reset(new GrammarAnnotation);

  try {
    auto const root = expectedPath(&doc, { "annotation" });
    parseBaseParameters(root);

    parseGrammar(expectedPath(root, { "grammar" }));

    parseLibrary(expectedPath(root, { "lib" }));

    parsePointsOfInterest(expectedPath(root, { "points-of-interest" }));

    parsePointcuts(expectedPath(root, { "pointcuts" }));

    // skip everything else
  } catch (string const msg) {
    SCIS_ERROR("Incorrect grammar annotation. " << msg);
  }

  return std::move(annotation);
}
