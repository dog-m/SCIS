#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "logging.h"
#include <string>

namespace txl {

  using namespace std;

  constexpr string_view NEW_LINE_TAG = "x-new-line-x";

  struct Interpreter
  {
    static void test();

    static string grammarToXML(string_view const& filename);

    static string rulesetToXML(string_view const& filename);
  };

} // TXL

#endif // INTERPRETER_H
