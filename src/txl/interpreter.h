#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "../logging.h"
#include <string>

namespace TXL {

  struct TXLInterpreter
  {
    static void test();

    static std::string grammarToXML(std::string const &grammarFileName);
  };

}

#endif // INTERPRETER_H
