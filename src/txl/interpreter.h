#ifndef TXLINTERPRETER_H
#define TXLINTERPRETER_H

#include "../logging.h"
#include <string>

namespace TXL {

  using namespace std;

  constexpr string_view NEW_LINE_TAG = "x-new-line-x";

  struct TXLInterpreter
  {
    static void test();

    static std::string grammarToXML(std::string const &grammarFileName);
  };

}

#endif // TXLINTERPRETER_H
