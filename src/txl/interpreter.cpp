#include "interpreter.h"
#include "wrapper.h"

using namespace std;

void TXL::TXLInterpreter::test() {
  TXL::TXLWrapper::runNoInput({ "-v" },
                              TXL::TXLWrapper::NOOP_READER,
                              [](string const& line){
    cout << line << endl;
    return true;
  });
}

string TXL::TXLInterpreter::grammarToXML(string const &grammarFileName) {
  constexpr auto GRAMMAR_TREE_START = "-- Grammar Tree --";
  constexpr auto GRAMMAR_TREE_END   = "-- End Grammar Tree --";

  string result = "";
  bool recordingXML = false;

  const auto stdErrReader = [&](string const& line) {
    if (line.find(GRAMMAR_TREE_START) != string::npos) {
      recordingXML = true;
      return true;
    }
    else
      if (line.find(GRAMMAR_TREE_END) != string::npos) {
        recordingXML = false;
        return false;
      }

    if (recordingXML)
      result += line;

    return true;
  };

  TXL::TXLWrapper::runNoInput({ "#", grammarFileName, "-Dgrammar" }, TXL::TXLWrapper::NOOP_READER, stdErrReader);

  if (recordingXML)
    SCIS_WARNING("Unexpected end of grammar tree");

  return result;
}
