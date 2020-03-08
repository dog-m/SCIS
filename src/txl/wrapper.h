#ifndef TXLWRAPPER_H
#define TXLWRAPPER_H

#include <functional>
#include <vector>

namespace TXL {

  using namespace std;

  using ReaderFunction = std::function< bool(string const&) >;

  struct Wrapper final
  {
    static bool NOOP_READER(string const&);

    static void runNoInput(initializer_list<string_view> && params,
                           ReaderFunction const& errReader,
                           ReaderFunction const& outReader = NOOP_READER);
  private:
    Wrapper() = delete;
  };

}

#endif // TXLWRAPPER_H
