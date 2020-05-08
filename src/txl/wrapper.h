#ifndef WRAPPER_H
#define WRAPPER_H

#include <functional>
#include <vector>

namespace txl {

  using namespace std;

  using ReaderFunction = std::function< bool(string const&) >;

  struct Wrapper final
  {
    static bool NOOP_READER(string const&);
    
    static bool LOG_READER(string const& line);

    static ReaderFunction STRING_READER(string& out);

    static int runNoInput(initializer_list<string_view> && params,
                          ReaderFunction const& errReader,
                          ReaderFunction const& outReader = NOOP_READER);

    static int runShellCommand(string const& command,
                               ReaderFunction const& errReader,
                               ReaderFunction const& outReader = NOOP_READER);
  private:
    Wrapper() = delete;
  };

} // TXL

#endif // WRAPPER_H
