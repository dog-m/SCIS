#include "logging.h"

#include <tinyxml2/tinyxml2.h>
#include <fstream>

#include "txl/grammar_parser.h"
#include "scis/ruleset_parser.h"
#include "scis/annotation_parser.h"
#include "scis/txl_generator.h"
#include "scis/arguments.h"

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

int main(int argc, char** argv)
{
  scis::args::updateArguments(argc, argv);

  scis::TXLGenerator generator;
  SCIS_INFO("Loading grammar");
  generator.grammar = loadAndParseGrammar(scis::args::ARG_GRAMMAR);
  /*for (auto const& [_, type] : generator.grammar->types)
    type->toTXLDefinition(cout);
  generator.grammar->toDOT(cout);*/

  SCIS_INFO("Loading annotation");
  generator.annotation = loadAndParseAnnotation(scis::args::ARG_ANNOTATION);
  //generator.annotation->dump(cout);

  SCIS_INFO("Loading ruleset");
  generator.ruleset = loadAndParseRuleset(scis::args::ARG_RULESET);
  //generator.ruleset->dump(cout);

  generator.processingFilename = scis::args::ARG_SRC_FILENAME;
  generator.fragmentsDir = scis::args::ARG_FRAGMENTS_DIR;

  SCIS_INFO("Building...");
  generator.compile();

  ofstream outputFile("./example/example-generated.txl");
  SCIS_INFO("Generating code...");
  generator.generateCode(outputFile);

  scis::args::ARG_DST_FILENAME;

  SCIS_INFO("All done");
  return 0;
}
