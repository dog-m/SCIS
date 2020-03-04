#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <stack>
#include <queue>
#include <deque>

#include "tinyxml2/tinyxml2.h"

#include "logging.h"
#include "txl/interpreter.h"
#include "txl/grammar.h"
#include "txl/grammar_parser.h"

using namespace std;
using namespace tinyxml2;

#define NODISCARD [[nodiscard]]

/* =================================================================================== */



template<typename T>
struct auto_pool {
  using pool_item = T*;

  auto_pool() = default;
  auto_pool(auto_pool &&) = delete;
  auto_pool(auto_pool const&) = delete;

  ~auto_pool() {
    for (auto const item : items)
      delete item;
  }

  template<typename ...Args>
  NODISCARD
  inline pool_item getNew(Args&&... args) {
    auto const item = new T(forward<Args>(args)...);
    return items.emplace_back(item);
  }

  template<typename U, typename ...Args>
  NODISCARD
  inline U* getNewTyped(Args&&... args) {
    return static_cast<U*>(getNew(forward<Args>(args)...));
  }

private:
  vector<pool_item> items;
};



/* =================================================================================== */



using Label = int;

constexpr Label LABEL_INCORRECT = Label(-1);

enum class Kind {
  Order,
  Choose,
  Repeat,
  List
};

struct TypeNode {
  string_view name;
  Label label;
  Kind kind;
  unordered_set<TypeNode*> pointingTo, pointingFrom;

  TypeNode* shortestPathToRoot = nullptr;
  uint16_t distanceToRoot = decltype(distanceToRoot)(-1);
  bool isWeak = false;

  TypeNode(string_view const name, Label const label, Kind const kind):
    name(name), label(label), kind(kind), shortestPathToRoot(nullptr) {
    // check if node have weak connections
    isWeak = false;
    if (auto const pos = name.find_first_of('_'); pos != string_view::npos) {
      auto const prefix = name.substr(0, pos);
      if (prefix == "opt" || (prefix == "list" && name.back() != '+'))
        isWeak = true;
    }
  }

  void pointTo(TypeNode *const other) {
    if (other != this) {
      pointingTo.insert(other);
      other->pointingFrom.insert(this);
    }
  }

  string renderName() const {
    return "<" + to_string(label) + " " + name.data() + ">";
  }
};



struct TypeGraph {
  auto_pool<TypeNode> pool;
  unordered_map<string_view, TypeNode*> types;

  vector<TypeNode*> __xmlOrderedNodes;

  TypeGraph() = default;
  TypeGraph(TypeGraph &&) = delete;
  TypeGraph(TypeGraph const&) = delete;

  auto createNode(Label const label,
                  string_view const type,
                  Kind const kind) {
    return types[type] = pool.getNew(type, label, kind);
  }

  NODISCARD
  TypeNode* findNodeByName(string_view const name) const noexcept {
    if (auto const it = types.find(name); it != types.cend())
      return it->second;
    else
      return nullptr;
  }
};



/* =================================================================================== */



static unordered_set<string_view> const TXL_SPECIAL {
  "NL", "IN", "EX", "SPON", "SPOFF",
  "empty", "id", "number", "charlit", "stringlit"
};

struct TypeGraphBuilder: public XMLVisitor {
  using NodePtr = TypeNode*;

  TypeGraph* graph;
  NodePtr currentNode = nullptr;
  stack<NodePtr> roots;
  Label expectedLabel = 0;
  stack<pair<uint32_t, NodePtr>> hangingNodes;
  uint32_t xmlTreeDepth = 0;

  unordered_map<Label, NodePtr> referenceMap;

  NODISCARD
  NodePtr registerNode(Label const label,
                       string_view const name,
                       string_view const kindStr) {
    Kind kind = Kind::Order;
         if (kindStr == "list")   kind = Kind::List;
    else if (kindStr == "choose") kind = Kind::Choose;
    else if (kindStr == "repeat") kind = Kind::Repeat;

    /*auto node = graph->findNodeByName(name);
    if (!node) {*/
      auto node = graph->createNode(label, name, kind);
      graph->__xmlOrderedNodes.push_back(node);
    //}

    return referenceMap[label] = node;
  }

  NODISCARD
  NodePtr findNodeByReference(Label const ref) {
    return referenceMap[ref];
  }

  bool VisitEnter(XMLElement const& element,
                  XMLAttribute const *const) override {
    string_view tagName = element.Name();
    Label label = element.IntAttribute("label", LABEL_INCORRECT);
    Label ref = element.IntAttribute("ref", LABEL_INCORRECT);

    bool is_labeled = (label != LABEL_INCORRECT);
    bool const is_reference = (ref != LABEL_INCORRECT);
    bool const is_special = (TXL_SPECIAL.find(tagName) != TXL_SPECIAL.cend());

    // set a new root for further nodes
    roots.push(currentNode);

    if (is_special) {
      currentNode = nullptr;
    }
    else {
      if (!is_reference)
        ++expectedLabel;

      if (!is_labeled && !is_reference && !is_special) {
        label = expectedLabel;
        //WARNING("Non-indexed node type: " << tagName << ". It probably have label = " << label);
        is_labeled = true;
      }

      if (is_labeled && label != expectedLabel)
        SCIS_WARNING("Desynchronization with expected label counter: expected " << expectedLabel << ", actual " << label);

      currentNode = is_reference ? findNodeByReference(ref) : registerNode(label, tagName, element.Attribute("kind"));
      if (!currentNode) {
        SCIS_ERROR("Undefined reference " << ref);
        terminate();
      }

      hangingNodes.push(make_pair(xmlTreeDepth, currentNode));
    }

    ++xmlTreeDepth;

    return true;
  }

  bool VisitExit(XMLElement const&) override {
    --xmlTreeDepth;

    auto root = roots.top();
    if (root) {
      // TODO: add connection logic
      while (!hangingNodes.empty() && hangingNodes.top().first == xmlTreeDepth) {
        if (hangingNodes.top().second)
          root->pointTo(hangingNodes.top().second);

        hangingNodes.pop();
      }
    }


    currentNode = roots.top();
    roots.pop();

    return true;
  }

  void reset(TypeGraph *const graphToBuild) {
    graph = graphToBuild;
    currentNode = nullptr;
    while (!roots.empty()) roots.pop();
    expectedLabel = 0;
    while (!hangingNodes.empty()) hangingNodes.pop();
    xmlTreeDepth = 0;
    referenceMap.clear();

    graphToBuild->__xmlOrderedNodes.clear();
  }

  void buildGraph(XMLDocument const& xmlDoc,
                  TypeGraph *const graphToBuild) {
    reset(graphToBuild);

    xmlDoc.Accept(this);

    if (auto const count = hangingNodes.size(); count == 0)
      SCIS_ERROR("Expected top node type to hang around");

    else if (count == 1 && hangingNodes.top().second->name == "program")
      hangingNodes.pop();

    else {
      SCIS_ERROR("Something left behind!");
      while (!hangingNodes.empty()) {
        cerr << hangingNodes.top().second->name << endl;
        hangingNodes.pop();
      }
    }
  }

};



/* =================================================================================== */



static void renderAsDOT(TypeGraph const& g) {
  decltype(TypeNode::distanceToRoot) maxDistance = 0;
  for (auto const type : g.__xmlOrderedNodes)
    maxDistance = max(maxDistance, type->distanceToRoot);

  cout << "digraph G {" << endl;

  for (auto const type : g.__xmlOrderedNodes)
    cout << "  " << type->renderName()
         //<< (type->name == "program" ? " [fillcolor=\"0.0 0.35 1.0\" style=filled];" : "")
         << " [fillcolor=\"" << (type->distanceToRoot * 0.75f / maxDistance) << " 0.35 1.0\" style=filled];"
         << endl;

  cout << endl;

  for (auto const type : g.__xmlOrderedNodes) {
    cout << "  " << type->renderName() << " -> { ";

    int shouldBePrinted = static_cast<int>(type->pointingTo.size());
    for (auto const point : type->pointingTo)
      cout << point->renderName() << (shouldBePrinted --> 1 ? ", " : "");

    cout << " }"
         << (type->isWeak ? " [style=dotted constraint=false];" : "")
         << endl;
  }

  cout << "}" << endl;
}



/* =================================================================================== */



static void parse(XMLDocument &doc,
                  string const& grammarXML,
                  TypeGraph &graph) {
  if (auto result = doc.Parse(grammarXML.data()); result != XML_SUCCESS) {
    SCIS_ERROR("Loading failed: " << doc.ErrorIDToName(result));
    return;
  }
  else
    SCIS_INFO("XML loaded normaly");

  TypeGraphBuilder builder;
  builder.buildGraph(doc, &graph);
}



/* =================================================================================== */



using NodeRefC = TypeNode const*;

static void printAllPaths_DFS(NodeRefC const A,
                              NodeRefC const B,
                              unordered_set<NodeRefC> &visited,
                              list<NodeRefC> &path) {
  visited.insert(A);
  path.push_back(A);

  if (A == B) {
    // just print it
    // TODO: save in result list
    for (auto const point : path)
      cout << point->name << " -> ";
    cout << endl;
  }
  else {
    // just go through each neighbour
    for (auto const nextPoint : A->pointingTo)
      if (visited.find(nextPoint) == visited.cend())
        printAllPaths_DFS(nextPoint, B, visited, path);
  }

  path.pop_back();
  visited.erase(A);
}

static void printAllPaths(NodeRefC const A,
                          NodeRefC const B) {
  unordered_set<NodeRefC> visited;
  list<NodeRefC> path;
  printAllPaths_DFS(A, B, visited, path);
}



/* =================================================================================== */



static void rebuildShortestPathsBFS(TypeGraph &graph,
                                    TypeNode *const start) {
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
}



/* =================================================================================== */

int main(/*int argc, char** argv*/) {
  /*
  TypeGraph graph;
  XMLDocument doc;
  SCIS_INFO("Parsing...");
  parse(doc, TXL::TXLInterpreter::grammarToXML("./example/lang/java/grammar.txl"), graph);

  SCIS_INFO("Building shortest paths...");
  rebuildShortestPathsBFS(graph, graph.types.at("program"));

  SCIS_INFO("Rendering...");
  renderAsDOT(graph);

  //INFO("Looking for paths between <program> and <class_header>");
  //printAllPaths(graph.types.at("program"), graph.types.at("class_header"));
  */
  auto const grammarXMLSource = TXL::TXLInterpreter::grammarToXML("./example/lang/java/grammar.txl");
  SCIS_INFO("Grammar size: " << grammarXMLSource.size());

  XMLDocument doc(true, COLLAPSE_WHITESPACE);
  doc.Parse(grammarXMLSource.data());

  SCIS_INFO("loaded");

  TXL::TXLGrammarParser parser;
  auto const grm = parser.parse(doc);

  SCIS_INFO("As TXL:");
  for (auto const& [_, type] : grm->types)
    type->toString(cout);

  return 0;
}
