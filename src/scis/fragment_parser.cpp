#include "fragment_parser.h"
#include "logging.h"
#include <sstream>

using namespace std;
using namespace scis;
using namespace tinyxml2;

#include "../xml_parser_utils.h"

void FragmentParser::parseDependencies(XMLElement const* const dependencies)
{
  FOREACH_XML_ELEMENT(dependencies, dep) {
    auto& dependency = fragment->dependencies.emplace_back(/* empty */);

    dependency.target = expectedAttribute(dep, "name")->Value();
    // TODO: add more complicated dependencies
    dependency.required = (dep->Name() == "required"sv);
  }
}

void FragmentParser::parseCode(XMLElement const* const code)
{
  // base params
  if (auto const black_list = code->FindAttribute("black-list"))
    processList(',', black_list->Value(), [&](string const& id) {
      fragment->black_list.push_back(id);
    });
  if (auto const params = code->FindAttribute("params"))
    processList(',', params->Value(), [&](string const& id) {
      fragment->params.push_back(id);
    });

  // blocks of source code with references
  FOREACH_XML_NODE(code, block) {
    // just text?
    if (auto text = block->ToText()) {
      string line;
      stringstream ss(text->Value());

      vector<string> lines;
      while (!ss.eof()) {
        // trim left
        while (!ss.eof() && isspace(ss.peek()))
          ss.ignore();

        // read line-by-line and create blocks of text
        if (getline(ss, line)) {
          // trim right
          while (!line.empty() && isblank(line.back()))
            line.pop_back();

          lines.push_back(line);
        }
      }

      for (size_t index = 0; index < lines.size(); index++) {
        bool const isLast = (index+1 == lines.size());

        auto txt = make_unique<Fragment::TextBlock>();
        txt->text = lines[index] + (isLast ? "" : "\n");
        fragment->source.emplace_back(std::move(txt));
      }
    }

    auto node = block->ToElement();
    // parameter reference
    if (node && node->Name() == "p"sv) {
      auto ref = make_unique<Fragment::ParamReference>();
      ref->id = expectedAttribute(node, "id")->Value();
      fragment->source.emplace_back(std::move(ref));
    }
  }

  // remove unnecessary empty row at the end
  if (!fragment->source.empty())
    if (auto const plaintext = dynamic_cast<Fragment::TextBlock*>(fragment->source.back().get())) {
      if (plaintext->text.empty())
        fragment->source.pop_back();
    }
}

void FragmentParser::parseFragment(XMLElement const* const root)
{
  // base params
  fragment->language = expectedAttribute(root, "language")->Value();
  fragment->name = expectedAttribute(root, "name")->Value();

  parseDependencies(expectedPath(root, { "dependencies" }));

  parseCode(expectedPath(root, { "code" }));
}

unique_ptr<Fragment> FragmentParser::parse(XMLDocument const&doc)
{
  fragment.reset(new Fragment);

  try {
    parseFragment(expectedPath(&doc, { "fragment" }));

    // skip everything else
  } catch (string const msg) {
    SCIS_ERROR("Incorrect fragment. " << msg);
  }

  return std::move(fragment);
}

unique_ptr<Fragment> FragmentParser::parse(const string_view& filename)
{
  XMLDocument doc(true, PRESERVE_WHITESPACE);
  if (auto result = doc.LoadFile(filename.data()); result != XML_SUCCESS) {
    SCIS_ERROR("Failed to load \'" << filename << "\'. "
               "Reason: " << doc.ErrorIDToName(result));
  }
  else
    SCIS_DEBUG("XML loaded normaly");

  return parse(doc);
}
