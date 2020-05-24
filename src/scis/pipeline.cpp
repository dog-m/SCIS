#include "pipeline.h"
#include "logging.h"
#include "cli_arguments.h"

#include "../txl/wrapper.h"
#include "../xml_parser_utils.h"

#include <fstream>

using namespace std;
using namespace scis;

string pipeline::preparePipeline(
    string const& pipeline,
    string const& outTxlFile)
{
  // combine params
  string params;
  params += txl::PARAM_INCLUDE_DIR + '\"' + scis::args::ARG_ANNOTATION_DIR + "\" ";
  //params += txl::PARAM_VERBOSE;
  params += scis::args::ARG_TXL_PARAMETERS;

  // build-up shell command by replacing placeholders
  string cmd = pipeline;
  cmd = replace_all(cmd, "%WORKING_DIR%"    , scis::args::ARG_WORKING_DIR     );
  cmd = replace_all(cmd, "%ANNOTATON_DIR%"  , scis::args::ARG_ANNOTATION_DIR  );
  cmd = replace_all(cmd, "%SRC%"            , scis::args::ARG_SRC_FILENAME    );
  cmd = replace_all(cmd, "%DST%"            , scis::args::ARG_DST_FILENAME    );
  cmd = replace_all(cmd, "%TRANSFORM%"      , outTxlFile                      );
  cmd = replace_all(cmd, "%PARAMS%"         , params                          );
  return cmd;
}

void pipeline::runPipeline(string const& preparedPipeline)
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
    SCIS_INFO("Ok");
}
