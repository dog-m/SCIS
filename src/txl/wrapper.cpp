#include "wrapper.h"

using namespace std;

/// https://stackoverflow.com/a/59338759
// Workaround for a boost/mingw bug.
// This must occur before the inclusion of the boost/process.hpp header.
// Taken from https://github.com/boostorg/process/issues/96
#ifndef __kernel_entry
  #define __kernel_entry
#endif
#include <boost/process.hpp>

using namespace boost::process;

void TXL::TXLWrapper::runNoInput(const vector<string> &params,
                                 const TXL::ReaderFunction &outReader,
                                 const TXL::ReaderFunction &errReader)
{
  ipstream outStream, errStream;
  child c(search_path("txl"), args(params), std_in.close(), std_out > outStream, std_err > errStream);

  string line;
  while (/*c.running() &&*/ getline(errStream, line) /*&& !line.empty()*/)
    if (!errReader(line))
      break;

  while (/*c.running() &&*/ getline(outStream, line) /*&& !line.empty()*/)
    if (!outReader(line))
      break;

  c.wait();
}
