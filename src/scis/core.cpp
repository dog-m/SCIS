#include "core.h"
#include "logging.h"
#include "cli_arguments.h"
#include "caching.h"
#include "annotation_parser.h"
#include "ruleset_parser.h"
#include "txl_generator.h"
#include "pipeline.h"

#include "../xml_parser_utils.h"

#include "../txl/grammar_parser.h"
#include "../txl/wrapper.h"

#include <tinyxml2/tinyxml2.h>
#include <fstream>

using namespace std;
using namespace scis;
using namespace tinyxml2;


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

  return tryParse<RulesetParser>(xml.data());
}

static auto loadAndParseAnnotation(string_view && filename)
{
  return tryLoadAndParse<AnnotationParser>(filename.data());
}


static void generateTXLinstructions(
    string const& outTxlFile,
    caching::CacheSignData const& signData,
    GrammarAnnotation* const annotation)
{
  ofstream outputFile(outTxlFile, ios::trunc);
  if (!outputFile.is_open())
    SCIS_ERROR("Failed to create/rewrite transformation source");

  TXLGenerator generator;
  SCIS_INFO("Loading grammar");
  generator.grammar = loadAndParseGrammar(args::ARG_GRAMMAR);

  // annotation is already loaded
  generator.annotation = annotation;

  SCIS_INFO("Loading ruleset");
  generator.ruleset = loadAndParseRuleset(args::ARG_RULESET);
  generator.ruleset->applyUserBlacklist(args::ARG_DISABLED_RULES);

  generator.fragmentsDir = args::ARG_FRAGMENTS_DIR;

  SCIS_INFO("Building...");
  generator.compile();

  // place cache-specific mark on it
  caching::applyCacheFileSign(outputFile, signData);
  outputFile << endl;

  SCIS_INFO("Generating code...");
  generator.generateCode(outputFile);

  SCIS_INFO("Generated TXL code saved as [" << outTxlFile << "]");
}


void core::doTheWork()
{
  auto const outTxlFile = caching::generateFilenameByRuleset(args::ARG_RULESET);

  SCIS_INFO("Loading annotation");
  auto const annotation = loadAndParseAnnotation(args::ARG_ANNOTATION);
  args::updateGrammarLocation(annotation->grammar.txlSourceFilename);

  //=====
  // TODO: remove debug output
  SCIS_DEBUG("wrkdir: >" << args::ARG_WORKING_DIR          << "<");
  SCIS_DEBUG("exedir: >" << args::ARG_EXECUTABLE_DIR       << "<");
  SCIS_DEBUG("in-txl: >" << args::ARG_INTERNAL_GRM_TXL     << "<");
  SCIS_DEBUG("in-rul: >" << args::ARG_INTERNAL_GRM_RULESET << "<");

  SCIS_DEBUG("src:    >" << args::ARG_SRC_FILENAME   << "<");
  SCIS_DEBUG("dst:    >" << args::ARG_DST_FILENAME   << "<");
  SCIS_DEBUG("ann:    >" << args::ARG_ANNOTATION     << "<");
  SCIS_DEBUG("anndir: >" << args::ARG_ANNOTATION_DIR << "<");
  SCIS_DEBUG("grm:    >" << args::ARG_GRAMMAR        << "<");
  SCIS_DEBUG("rules:  >" << args::ARG_RULESET        << "<");
  SCIS_DEBUG("fragdi: >" << args::ARG_FRAGMENTS_DIR  << "<");
  SCIS_DEBUG("cache:  >" << args::ARG_USE_CACHE      << "<");
  SCIS_DEBUG("txlpar: >" << args::ARG_TXL_PARAMETERS << "<");

  if (args::ARG_DISABLED_RULES.empty())
    SCIS_DEBUG("d-rule: 0");
  else
    for (auto const& drule : args::ARG_DISABLED_RULES)
      SCIS_DEBUG("d-rule: >" << drule << "<");

  //=====

  auto const data = caching::initCacheSign(args::ARG_RULESET, annotation->grammar.language);

  bool const cached = args::ARG_USE_CACHE && caching::checkCache(outTxlFile, data);
  if (!cached) {
    SCIS_INFO("There are no cached results for a particular ruleset. Generating TXL instructions...");
    generateTXLinstructions(outTxlFile, data, annotation.get());
  }

  // prepare and run instrumentation command
  auto const cmd = pipeline::preparePipeline(annotation->pipeline, outTxlFile);
  SCIS_DEBUG("Prepared command:" << endl << cmd);
  pipeline::runPipeline(cmd);
}
