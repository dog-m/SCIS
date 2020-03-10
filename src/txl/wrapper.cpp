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
#include <boost/asio/io_service.hpp>
#include <future>
#include <sstream>

#include "logging.h"

using namespace boost::process;
using namespace txl;

bool Wrapper::NOOP_READER(string const&)
{
  return true;
}

void Wrapper::runNoInput(initializer_list<string_view> && params,
                            const ReaderFunction &errReader,
                            const ReaderFunction &outReader)
{
  static auto const txl_executable = search_path("txl");

//  boost::asio::io_service ios;
//  ipstream outStream, errStream;
//  child c(txl_executable, args(params), std_in.close(), std_out > outStream, std_err > errStream);

//  string line;
//  do {
//    while (/*c.running() &&*/ errStream && !errStream.eof() && getline(errStream, line) /*&& !line.empty()*/)
//      if (!errReader(line))
//        break;

//    while (/*c.running() &&*/ outStream && !outStream.eof() && getline(outStream, line) /*&& !line.empty()*/)
//      if (!outReader(line))
//        break;
//  } while (c.running() && false);

//  c.wait();
  boost::asio::io_service ios;

  future<string> data_err, data_out;

  child c(txl_executable, args(params),
          std_in.close(),
          std_out > data_out,
          std_err > data_err,
          ios);

  // blocked until finished
  ios.run();

  string line;
  stringstream ss;

  // read errors (std_err)
  ss.str(data_err.get());
  while (getline(ss, line))
    if (!errReader(line))
      break;

  // read text (std_out)
  ss.clear();
  ss.str(data_out.get());
  while (getline(ss, line))
    if (!outReader(line))
      break;
}
