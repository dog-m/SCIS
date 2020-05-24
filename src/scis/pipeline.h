#ifndef PIPELINE_H
#define PIPELINE_H

#include <string>

namespace scis::pipeline {

  using namespace std;

  string preparePipeline(string const& pipeline,
                         string const& outTxlFile);

  void runPipeline(string const& preparedPipeline);

}

#endif // PIPELINE_H
