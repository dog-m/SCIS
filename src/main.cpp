#include "logging.h"

#include <tinyxml2/tinyxml2.h>
#include <fstream>

#include "txl/grammar_parser.h"
#include "scis/ruleset_parser.h"
#include "scis/annotation_parser.h"
#include "scis/txl_generator.h"
#include "scis/cli_arguments.h"
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

  if (xml.size() == 0)
    SCIS_ERROR("Failed to load grammar. Did you forget txl grammar?");

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
    scis::caching::CacheSignData const& signData,
    scis::GrammarAnnotation* const annotation)
{
  ofstream outputFile(outTxlFile, ios::trunc);
  if (!outputFile.is_open())
    SCIS_ERROR("Failed to create/rewrite transformation source");

  scis::TXLGenerator generator;
  SCIS_INFO("Loading grammar");
  generator.grammar = loadAndParseGrammar(scis::args::ARG_GRAMMAR);

  // annotation is already loaded
  generator.annotation = annotation;

  SCIS_INFO("Loading ruleset");
  generator.ruleset = loadAndParseRuleset(scis::args::ARG_RULESET);
  generator.ruleset->applyUserBlacklist(scis::args::ARG_DISABLED_RULES);

  generator.fragmentsDir = scis::args::ARG_FRAGMENTS_DIR;

  SCIS_INFO("Building...");
  generator.compile();

  // place cache-specific mark on it
  scis::caching::applyCacheFileSign(outputFile, signData);
  outputFile << endl;

  SCIS_INFO("Generating code...");
  generator.generateCode(outputFile);

  SCIS_INFO("Generated TXL code saved as [" << outTxlFile << "]");
}


// TODO: move to a separate location [pipelining]
static string preparePipeline(
    string const& pipeline,
    string const& outTxlFile)
{
  // combine params
  string params;
  params += txl::PARAM_INCLUDE_DIR + "\"" + scis::args::ARG_ANNOTATION_DIR + "\" ";
  //params += txl::PARAM_VERBOSE;
  params += scis::args::ARG_TXL_PARAMETERS;

  // build-up shell command by replacing placeholders
  string cmd = pipeline;
  cmd = replace_all(cmd, "%WORKDIR%"  , scis::args::ARG_WORKING_DIR     );
  cmd = replace_all(cmd, "%SRC%"      , scis::args::ARG_SRC_FILENAME    );
  cmd = replace_all(cmd, "%DST%"      , scis::args::ARG_DST_FILENAME    );
  cmd = replace_all(cmd, "%TRANSFORM%", outTxlFile                      );
  cmd = replace_all(cmd, "%PARAMS%"   , params                          );
  return cmd;
}


// TODO: move to a separate location [pipelining]
static void runPipeline(string const& preparedPipeline)
{
  // TODO: find way to get output of any shell command (including pipes)
  SCIS_INFO("Executing transformation pipeline...");
  //string rOut, rErr;
  auto const result = txl::Wrapper::runShellCommand(preparedPipeline/*,
                                                    txl::Wrapper::STRING_READER(rErr),
                                                    txl::Wrapper::STRING_READER(rOut)*/);
  if (result != 0) {
    SCIS_WARNING("Something went wrong. Return code = " << result);
    // render txl utility output
    //SCIS_INFO("TXL output:" << endl << rErr << rOut);

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

  auto const outTxlFile = scis::caching::generateFilenameByRuleset(scis::args::ARG_RULESET);

  SCIS_INFO("Loading annotation");
  auto const annotation = loadAndParseAnnotation(scis::args::ARG_ANNOTATION);
  scis::args::updateGrammarLocation(annotation->grammar.txlSourceFilename);

  //=====
  // TODO: remove debug output
  SCIS_DEBUG("workdi: >" << scis::args::ARG_WORKING_DIR          << "<");
  SCIS_DEBUG("execdi: >" << scis::args::ARG_EXECUTABLE_DIR       << "<");
  SCIS_DEBUG("in-txl: >" << scis::args::ARG_INTERNAL_GRM_TXL     << "<");
  SCIS_DEBUG("in-rul: >" << scis::args::ARG_INTERNAL_GRM_RULESET << "<");

  SCIS_DEBUG("src:    >" << scis::args::ARG_SRC_FILENAME   << "<");
  SCIS_DEBUG("dst:    >" << scis::args::ARG_DST_FILENAME   << "<");
  SCIS_DEBUG("annot:  >" << scis::args::ARG_ANNOTATION     << "<");
  SCIS_DEBUG("anndir: >" << scis::args::ARG_ANNOTATION_DIR << "<");
  SCIS_DEBUG("grm:    >" << scis::args::ARG_GRAMMAR        << "<");
  SCIS_DEBUG("rules:  >" << scis::args::ARG_RULESET        << "<");
  SCIS_DEBUG("disrul: >" << scis::args::ARG_DISABLED_RULES << "<");
  SCIS_DEBUG("frags:  >" << scis::args::ARG_FRAGMENTS_DIR  << "<");
  SCIS_DEBUG("cache:  >" << scis::args::ARG_USE_CACHE      << "<");
  SCIS_DEBUG("params: >" << scis::args::ARG_TXL_PARAMETERS << "<");

  //=====

  auto const data = scis::caching::initCacheSign(scis::args::ARG_RULESET, annotation->grammar.language);

  bool const cached = scis::args::ARG_USE_CACHE && scis::caching::checkCache(outTxlFile, data);
  if (!cached) {
    SCIS_INFO("There are no cached results for a particular ruleset. Generating TXL instructions...");
    generateTXLinstructions(outTxlFile, data, annotation.get());
  }

  // prepare and run instrumentation command
  auto const cmd = preparePipeline(annotation->pipeline, outTxlFile);
  SCIS_DEBUG("Prepared command:" << endl << cmd);
  runPipeline(cmd);

  return 0;
}
