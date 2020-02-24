#include "include/txlwrapper.h"

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

void TXLWrapper::test() {
  ipstream stream;
  child c(search_path("txl"), "-v", std_in.close(), std_out > null, std_err > stream);

  string line;
  while (/*c.running() &&*/ getline(stream, line) /*&& !line.empty()*/)
    cout << line << endl;

  c.wait();
}
