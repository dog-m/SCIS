#include "logging.h"

#include <tinyxml2/tinyxml2.h>
#include <fstream>

#include "txl/grammar_parser.h"
#include "scis/ruleset_parser.h"
#include "scis/annotation_parser.h"
#include "scis/txl_generator.h"

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

int main(/*int argc, char** argv*/)
{
  // FIXME: ===== debug purposes only =====

  string const lang = "java";

  auto const grammar = "./example/lang/" +lang+ "/grammar.txl";
  auto const annotation = "./example/lang/" +lang+ "/annotation.xml";
  auto const ruleset = "./example/add_logging_to_Main_main.yml";
  auto const fragDir = "./example/fragments/" +lang+ "/";

  // FIXME: ===== debug purposes only =====

  scis::TXLGenerator generator;
  SCIS_INFO("Loading grammar");
  generator.grammar = loadAndParseGrammar(grammar);
  /*for (auto const& [_, type] : generator.grammar->types)
    type->toTXLDefinition(cout);
  generator.grammar->toDOT(cout);*/

  SCIS_INFO("Loading annotation");
  generator.annotation = loadAndParseAnnotation(annotation);
  //generator.annotation->dump(cout);

  SCIS_INFO("Loading ruleset");
  generator.ruleset = loadAndParseRuleset(ruleset);
  //generator.ruleset->dump(cout);

  generator.processingFilename = "*dir/test." +lang+ "*";
  generator.fragmentsDir = fragDir;

  SCIS_INFO("Building...");
  generator.compile();

  ofstream outputFile("./example/example-generated.txl");
  SCIS_INFO("Generating code...");
  generator.generateCode(outputFile);

  SCIS_INFO("All done");
  return 0;
}
