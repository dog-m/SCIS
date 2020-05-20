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

inline static void read_line_by_line(
    string const& str,
    ReaderFunction const& reader)
{
  string line;
  stringstream ss(str);
  while (getline(ss, line))
    if (!reader(line))
      break;
}

bool Wrapper::READER_NOOP(string const&)
{
  return false;
}

bool Wrapper::READER_LOG(string const& line)
{
  cerr << line << endl;
  return true;
}

ReaderFunction Wrapper::STRING_READER(string& out)
{
  return [&out](string const& line){
      out += line + '\n';
      return true;
    };
}

int Wrapper::runNoInput(initializer_list<string_view> && params,
                        ReaderFunction const& errReader,
                        ReaderFunction const& outReader)
{
  static auto const txl_executable = search_path("txl");

  std::future<string> data_err, data_out;
  boost::asio::io_service ios;
  child c(txl_executable, args(params),
          std_in.close(),
          std_out > data_out,
          std_err > data_err,
          ios);

  // blocked until finished
  ios.run();

  // read errors (std_err) and text (std_out)
  read_line_by_line(data_err.get(), errReader);
  read_line_by_line(data_out.get(), outReader);

  // see https://stackoverflow.com/questions/59156417/getting-exit-code-of-boostprocesschild-under-boostasioio-service
  c.wait();
  return c.exit_code();
}

int Wrapper::runShellCommand(string const& command/*,
                             ReaderFunction const& errReader,
                             ReaderFunction const& outReader*/)
{
  /*boost::asio::io_service ios;
  std::future<string> data_err, data_out;
  auto ret = boost::process::system(command,
                                    //std_in.close(),
                                    std_out > data_out,
                                    std_err > data_err,
                                    ios);

  // block until finished
  ios.run();

  // read errors (std_err) and text (std_out)
  read_line_by_line(data_err.get(), errReader);
  read_line_by_line(data_out.get(), outReader);

  return ret;*/

  return std::system(command.data());
}
