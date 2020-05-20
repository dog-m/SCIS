#ifndef WRAPPER_H
#define WRAPPER_H

#include <functional>
#include <vector>

namespace txl {

  using namespace std;

  using ReaderFunction = std::function< bool(string const&) >;

  /// expecting a single space at the end of all parameters
  inline string PARAM_INCLUDE_DIR = "-i ";
  inline string PARAM_VERBOSE     = "-v ";
  inline string PARAM_HELP        = "-h ";
  inline string PARAM_XML_RESULT  = "-xml ";

  struct Wrapper final
  {
    static bool READER_NOOP(string const&);
    
    static bool READER_LOG(string const& line);

    static ReaderFunction STRING_READER(string& out);

    static int runNoInput(initializer_list<string_view> && params,
                          ReaderFunction const& errReader,
                          ReaderFunction const& outReader = READER_NOOP);

    static int runShellCommand(string const& command/*,
                               ReaderFunction const& errReader,
                               ReaderFunction const& outReader = NOOP_READER*/);
  private:
    Wrapper() = delete;
  };

} // TXL

#endif // WRAPPER_H
