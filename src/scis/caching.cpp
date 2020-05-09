#include "caching.h"
#include "arguments.h"

#include <fstream>
#include <boost/uuid/detail/sha1.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/filesystem/path.hpp>

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
  // fix (*-endian) order
  for (auto i = 0u; i < SIZE; i += 4) {
    swap(charDigest[i + 0u], charDigest[i + 3u]);
    swap(charDigest[i + 1u], charDigest[i + 2u]);
  }

  string result = "";
  // render as string of hexadecimals
  boost::algorithm::hex(charDigest, charDigest + SIZE, std::back_inserter(result));

  return result;
}

static string readFileToString(string const& filename)
{
  ifstream ruleset(filename, ios::binary);
  string text;

  ruleset.seekg(0, ios::end);
  text.reserve(ruleset.tellg());
  ruleset.seekg(0, ios::beg);

  text.assign((istreambuf_iterator<char>(ruleset)),
              istreambuf_iterator<char>());

  return text;
}

string caching::generateFilenameByRuleset(string const& rulesetFilename)
{
  string cacheDir = "./example/"; // TODO: move to CLI arguments

  string const name = boost::filesystem::path(rulesetFilename).filename().replace_extension("").string();

  return cacheDir + name + ".txl";
}

caching::CacheSignData caching::initCacheSign(
    string const& rulesetFilename,
    string const& language)
{
  return {
      .lang = language,
      .hash = getSHA1HashOf(readFileToString(rulesetFilename)),
    };
}

void caching::applyCacheFileSign(ostream& stream,
                                 CacheSignData const& data)
{
  // WARNING: single-line sign
  stream << "% Cache sign:"
            " {"
            " ver: "  << data.ver   << ","
            " lang: " << data.lang  << ","
            " hash: " << data.hash  << ","
            " }\n";
}

bool caching::checkCache(string const& outTxlFilename,
                         CacheSignData const& data)
{
  ifstream cachedFile(outTxlFilename);
  if (!cachedFile.good())
    return false;

  string line;
  // check a top row in a file
  if (!getline(cachedFile, line))
    return false;

  stringstream ss;
  applyCacheFileSign(ss, data);
  string const expectedText = ss.str();

  // fix last symbol
  line.push_back('\n');

  if (line != expectedText)
    return false;

  return true;
}
