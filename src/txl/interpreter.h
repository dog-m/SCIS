#ifndef TXLINTERPRETER_H
#define TXLINTERPRETER_H

#include "logging.h"
#include <string>

namespace txl {

  using namespace std;

  constexpr string_view NEW_LINE_TAG = "x-new-line-x";

  struct Interpreter
  {
    static void test();

    static string grammarToXML(string_view const& fileName);

    static string rulesetToXML(string_view const& fileName);
  };

} // TXL

#endif // TXLINTERPRETER_H
