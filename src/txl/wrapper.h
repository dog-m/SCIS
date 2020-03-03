#ifndef WRAPPER_H
#define WRAPPER_H

#include <functional>
#include <vector>

namespace TXL {

  using namespace std;

  using ReaderFunction = std::function< bool(string const&) >;

  struct TXLWrapper final
  {
    static auto NOOP_READER(string const&) { return false; }

    static void runNoInput(vector<string> const& params,
                           ReaderFunction const& outReader,
                           ReaderFunction const& errReader);
  private:
    TXLWrapper() = delete;
  };
}

#endif // WRAPPER_H
