#include "logging.h"

#include "txl/grammar_parser.h"
#include "scis/ruleset_parser.h"
#include "scis/annotation_parser.h"

#include "scis/txl_generator.h"

#include "tinyxml2/tinyxml2.h"

using namespace std;
using namespace tinyxml2;

/* =================================================================================== */



template<typename Parser>
static auto tryParse(const char* xml)
{
  XMLDocument doc(true, COLLAPSE_WHITESPACE);
  if (auto result = doc.Parse(xml); result != XML_SUCCESS) {
    SCIS_ERROR("XML Loading failed: " << doc.ErrorIDToName(result));
    terminate();
  }
  else
    SCIS_DEBUG("XML loaded normaly");

  Parser parser;
  return parser.parse(doc);
}

template<typename Parser>
static auto tryLoadAndParse(const char* fileName)
{
  XMLDocument doc(true, COLLAPSE_WHITESPACE);
  if (auto result = doc.LoadFile(fileName); result != XML_SUCCESS) {
    SCIS_ERROR("XML Loading failed: " << doc.ErrorIDToName(result));
    terminate();
  }
  else
    SCIS_DEBUG("XML loaded normaly");

  Parser parser;
  return parser.parse(doc);
}



static auto loadAndParseGrammar(string_view && filename)
{
  auto const xml = txl::Interpreter::grammarToXML(filename);
  SCIS_DEBUG("Grammar size: " << xml.size());

  return tryParse<txl::GrammarParser>(xml.data());
}

static auto loadAndParseRuleset(string_view && filename)
{
  auto const xml = txl::Interpreter::rulesetToXML(filename);
  SCIS_DEBUG("Ruleset size: " << xml.size());

  return tryParse<scis::RulesetParser>(xml.data());
}

static auto loadAndParseAnnotation(string_view && filename)
{
  return tryLoadAndParse<scis::AnnotationParser>(filename.data());
}



/* =================================================================================== */

//#include "xml_parser_utils.h"
//#include <sstream>

int main(/*int argc, char** argv*/)
{
  /*auto const grm = loadAndParseGrammar("./example/lang/java/grammar.txl");
  SCIS_INFO("Language grammar:");
  SCIS_INFO("As TXL:");
  for (auto const& [_, type] : grm->types)
    type->toTXLDefinition(cout);

  SCIS_INFO("As DOT:");
  grm->toDOT(cout);

  // ---

  auto const ruleset = loadAndParseRuleset("./example/add_logging_to_Main_main.yml");
  SCIS_INFO("Ruleset:");
  ruleset->dump(cout);

  // ---
  auto const annotation = loadAndParseAnnotation("./example/lang/java/annotation.xml");
  SCIS_INFO("Annotation:");
  annotation->dump(cout);*/

  scis::TXLGenerator generator;
  SCIS_DEBUG("Loading grammar");
  generator.grammar = loadAndParseGrammar("./example/lang/java/grammar.txl");

  SCIS_DEBUG("Loading annotation");
  generator.annotation = loadAndParseAnnotation("./example/lang/java/annotation.xml");

  SCIS_DEBUG("Loading ruleset");
  generator.ruleset = loadAndParseRuleset("./example/add_logging_to_Main_main.yml");

  generator.processingFilename = "*dir/test.java*";
  generator.fragmentsDir = "./example/fragments/java/";

  generator.compile();
  generator.generateCode(cout);

  return 0;
}
