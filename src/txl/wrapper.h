#ifndef TXLWRAPPER_H
#define TXLWRAPPER_H

#include <functional>
#include <vector>

namespace TXL {

  using namespace std;

  using ReaderFunction = std::function< bool(string const&) >;

  struct TXLWrapper final
  {
    static bool NOOP_READER(string const&) { return false; }

    static void runNoInput(vector<string> const& params,
                           ReaderFunction const& errReader,
                           ReaderFunction const& outReader = NOOP_READER);
  private:
    TXLWrapper() = delete;
  };

}

#endif // TXLWRAPPER_H
