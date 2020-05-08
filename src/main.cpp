#include "logging.h"

#include <tinyxml2/tinyxml2.h>
#include <fstream>

#include "txl/grammar_parser.h"
#include "scis/ruleset_parser.h"
#include "scis/annotation_parser.h"
#include "scis/txl_generator.h"
#include "scis/arguments.h"
#include "xml_parser_utils.h"
#include "txl/wrapper.h"
#include "scis/caching.h"

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


// TODO: move to a separate location [core]
static void generateTXLinstructions(
    string const& outTxlFile,
    scis::GrammarAnnotation* const annotation)
{
  scis::TXLGenerator generator;
  SCIS_INFO("Loading grammar");
  generator.grammar = loadAndParseGrammar(scis::args::ARG_GRAMMAR);

  // annotation is already loaded
  generator.annotation = annotation;

  SCIS_INFO("Loading ruleset");
  generator.ruleset = loadAndParseRuleset(scis::args::ARG_RULESET);

  generator.fragmentsDir = scis::args::ARG_FRAGMENTS_DIR;

  SCIS_INFO("Building...");
  generator.compile();

  ofstream outputFile(outTxlFile);
  SCIS_INFO("Generating code...");
  generator.generateCode(outputFile);

  SCIS_INFO("Generated TXL code saved as [" << outTxlFile << "]");
}


// TODO: move to a separate location [pipelining]
static string preparePipeline(
    string const& pipeline,
    string const& outTxlFile)
{
  string cmd = pipeline;
  cmd = replace_all(cmd, "%WORKDIR%"  , '\"' + scis::args::ARG_WORKING_DIR  + '\"'  );
  cmd = replace_all(cmd, "%SRC%"      , '\"' + scis::args::ARG_SRC_FILENAME + '\"'  );
  cmd = replace_all(cmd, "%DST%"      , '\"' + scis::args::ARG_DST_FILENAME + '\"'  );
  cmd = replace_all(cmd, "%TRANSFORM%", '\"' + outTxlFile                   + '\"'  );
  cmd = replace_all(cmd, "%PARAMS%"   , scis::args::ARG_TXL_PARAMETERS              );
  return cmd;
}


// TODO: move to a separate location [pipelining]
static void runPipeline(string const& preparedPipeline)
{
  string rOut, rErr;
  auto const result = txl::Wrapper::runShellCommand(preparedPipeline,
                                                    txl::Wrapper::STRING_READER(rErr),
                                                    txl::Wrapper::STRING_READER(rOut));
  if (result != 0) {
    SCIS_WARNING("Something went wrong. Return code = " << result);
    // render txl utility output
    SCIS_INFO("TXL output:" << endl << rErr << rOut);

    // copy source file to a destination
    ifstream a(scis::args::ARG_SRC_FILENAME, ios::binary);
    ofstream b(scis::args::ARG_DST_FILENAME, ios::binary);
    b << a.rdbuf();
    SCIS_INFO("Source file has been copied as a result");
  }
  else
    SCIS_INFO("Instrumented successfuly");
}


/* =================================================================================== */


int main(int argc, char** argv)
{
  scis::args::updateArguments(argc, argv);

  string const outTxlFile = scis::caching::generateFilenameByRuleset(scis::args::ARG_RULESET);

  SCIS_INFO("Loading annotation");
  auto const annotation = loadAndParseAnnotation(scis::args::ARG_ANNOTATION);

  bool const cached = scis::caching::checkCache(outTxlFile) && scis::args::ARG_USE_CACHE;
  if (!cached) {
    SCIS_INFO("There are no cached results for a particular ruleset. Generating TXL instructions...");
    generateTXLinstructions(outTxlFile, annotation.get());
  }

  // prepare and run instrumentation command
  string cmd = preparePipeline(annotation->pipeline, outTxlFile);
  SCIS_DEBUG("Prepared command:" << endl << cmd);
  runPipeline(cmd);

  return 0;
}
