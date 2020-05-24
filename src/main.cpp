#include "scis/cli_arguments.h"
#include "scis/core.h"

int main(int argc, char** argv)
{
  scis::args::updateArguments(argc, argv);

  scis::core::doTheWork();

  return 0;
}
