#include "logging.h"
#include "txl/interpreter.h"
#include "txl/grammar.h"
#include "txl/grammar_parser.h"

#include "tinyxml2/tinyxml2.h"

using namespace std;
using namespace tinyxml2;

#define NODISCARD [[nodiscard]]

/* =================================================================================== */



/*static void rebuildShortestPathsBFS(TypeGraph &graph,
                                    TypeNode *const start)
{
  using NodeRef = TypeNode*;

  unordered_set<NodeRef> visited;
  deque<pair<NodeRef, NodeRef>> queue;

  auto constexpr INF_DISTANCE = decltype(TypeNode::distanceToRoot)(-1);
  for (auto &[_, node] : graph.types) {
    node->distanceToRoot = INF_DISTANCE;
    node->shortestPathToRoot = nullptr;
  }

  queue.push_back(make_pair(nullptr, start));

  while (!queue.empty()) {
    auto const [root, node] = queue.front();
    queue.pop_front();

    decltype(INF_DISTANCE) const newDistance = root ? root->distanceToRoot + 1 : 0;
    if (newDistance < node->distanceToRoot) {
      node->distanceToRoot = newDistance;
      node->shortestPathToRoot = root;
    }
    visited.insert(node);

    for (auto const target : node->pointingTo)
      if (visited.find(target) == visited.cend())
        queue.push_back(make_pair(node, target));
  }
}*/



/* =================================================================================== */



static unique_ptr<txl::Grammar> loadAndParseGrammar(string_view && fileName)
{
  auto const grammarXMLSource = txl::Interpreter::grammarToXML(fileName);
  SCIS_DEBUG("Grammar size: " << grammarXMLSource.size());

  XMLDocument doc(true, COLLAPSE_WHITESPACE);
  if (auto result = doc.Parse(grammarXMLSource.data()); result != XML_SUCCESS) {
    SCIS_ERROR("XML Loading failed: " << doc.ErrorIDToName(result));
    terminate();
  }
  else
    SCIS_DEBUG("XML loaded normaly");

  txl::GrammarParser parser;
  return parser.parse(doc);
}



/* =================================================================================== */

int main(/*int argc, char** argv*/)
{
  auto const grm = loadAndParseGrammar("./example/lang/java/grammar.txl");

  SCIS_INFO("As TXL:");
  for (auto const& [_, type] : grm->types)
    type->toTXLDefinition(cout);

  SCIS_INFO("As DOT:");
  grm->toDOT(cout);

  return 0;
}
