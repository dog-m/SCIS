#include "interpreter.h"
#include "wrapper.h"

#include <sstream>

using namespace std;

void TXL::TXLInterpreter::test() {
  TXL::TXLWrapper::runNoInput({ "-v" },
                              [](string const& line){
    cout << line << endl;
    return true;
  });
}

string TXL::TXLInterpreter::grammarToXML(string const &grammarFileName) {
  constexpr auto GRAMMAR_TREE_START = "-- Grammar Tree --";
  constexpr auto GRAMMAR_TREE_END   = "-- End Grammar Tree --";

  stringstream result;
  bool recordingXML = false;

  TXL::TXLWrapper::runNoInput({ "#", grammarFileName, "-Dgrammar" }, [&](string const& line) {
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
      result << line << '<' << NEW_LINE_TAG << "/>" << endl;

    return true;
  });

  if (recordingXML)
    SCIS_WARNING("Unexpected end of grammar tree");

  return result.str();
}
