#include "caching.h"
#include "cli_arguments.h"

#include <fstream>
#include <boost/uuid/detail/sha1.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

using namespace std;
using namespace scis;


static string getHashOf(string const& str)
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

static string getTimestampOf(string const& filename)
{
  auto const timestamp = boost::filesystem::last_write_time(filename);
  return to_string(timestamp);
}



string caching::generateFilenameByRuleset(string const& rulesetFilename)
{
  auto const rulesetPath = boost::filesystem::path(rulesetFilename);

  // TODO: move caching folder to settings
  auto const cacheDir = rulesetPath.parent_path().string();

  auto const name = rulesetPath.filename().replace_extension("").string();

  return cacheDir + '/' + name + ".txl";
}

caching::CacheSignData caching::initCacheSign(
    string const& rulesetFilename,
    string const& language)
{
  return {
      .lang = language,
      .stamp = getTimestampOf(rulesetFilename),
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
            " stamp: " << data.stamp  << ","
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
