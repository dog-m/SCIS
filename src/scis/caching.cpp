#include "caching.h"
#include "arguments.h"

#include <fstream>
#include <boost/uuid/detail/sha1.hpp>
#include <boost/algorithm/hex.hpp>

using namespace std;
using namespace scis;


static string getSHA1HashOf(string const& str)
{
  using boost::uuids::detail::sha1;
  sha1 hash;
  sha1::digest_type digest;
  auto const SIZE = sizeof(decltype(digest));

  // evaluate hash
  hash.process_bytes(str.data(), str.size());
  hash.get_digest(digest);

  auto const charDigest = reinterpret_cast<char*>(digest);
  // fix order
  for (auto i = 0u; i < SIZE; i += 4) {
    swap(charDigest[i + 0u], charDigest[i + 3u]);
    swap(charDigest[i + 1u], charDigest[i + 2u]);
  }

  string result = "";
  // render as string of hexadecimals
  boost::algorithm::hex(charDigest, charDigest + SIZE, std::back_inserter(result));

  return result;
}

string caching::generateFilenameByRuleset(string const& rulesetFilename)
{
  ifstream ruleset(rulesetFilename, ios::binary);
  string text;

  ruleset.seekg(0, ios::end);
  text.reserve(ruleset.tellg());
  ruleset.seekg(0, ios::beg);

  text.assign((istreambuf_iterator<char>(ruleset)),
              istreambuf_iterator<char>());

  string cacheDir = "./example/"; // TODO: move to CLI arguments

  return cacheDir + getSHA1HashOf(text) + ".txl";
}

bool caching::checkCache(string const& outTxlFilename)
{
  ifstream cachedFile(outTxlFilename);
  return cachedFile.good();
}
