#ifndef CLI_H
#define CLI_H

#include <string>
#include <filesystem>

namespace scis
{
  using namespace std;

  struct CLI {

    filesystem::path srcFile;
    filesystem::path dstFile;
    filesystem::path ruleset;
    filesystem::path grammar;
    filesystem::path annotation;
    filesystem::path fragmentsDir;
    string disabledRules; // TODO: set of strings
    bool cacheEnabled = true;
    string txlParams;

    void parseArguments(int const argc,
                        char** const argv);

  };

} // scis

#endif // CLI_H
