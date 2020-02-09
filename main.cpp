#include <iostream>
#include "3rdparty/tinyxml2/tinyxml2.h"
#include <map>
#include <set>
#include <stack>
#include <vector>

using namespace std;
using namespace tinyxml2;

#define INFO(I)    (cout << "[INFO] " << I << endl)
#define WARNING(W) (cerr << "[WARNING] " << W << endl)
#define ERROR(E)   (cerr << "[ERROR] " << E << endl)

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
  [[nodiscard]]
  inline pool_item getNew(Args&&... args) {
    auto const item = new T(forward<Args>(args)...);
    return items.emplace_back(item);
  }

  template<typename U, typename ...Args>
  [[nodiscard]]
  inline U* getNewTyped(Args&&... args) {
    return static_cast<U*>(getNew(forward<Args>(args)...));
  }

private:
  vector<pool_item> items;
};



/* =================================================================================== */



using Label = int;

constexpr Label LABEL_INCORRECT = Label(-1);

struct TypeNode {
  Label label;
  string_view name;
  set<TypeNode*> references;

  TypeNode(Label const label, string_view const name):
    label(label), name(name) {}

  void connectTo(TypeNode *const other) {
    if (other != this)
      references.insert(other);
  }
};



struct TypeGraph {
  auto_pool<TypeNode> pool;
  map<string_view, TypeNode*> types;

  vector<TypeNode*> __xmlOrderedNodes;

  TypeGraph() = default;
  TypeGraph(TypeGraph &&) = delete;
  TypeGraph(TypeGraph const&) = delete;

  auto createNode(Label const label, string_view const type) {
    return types[type] = pool.getNew(label, type);
  }

  [[nodiscard]]
  TypeNode* findNodeByName(string_view const name) const noexcept {
    if (auto const it = types.find(name); it != types.cend())
      return it->second;
    else
      return nullptr;
  }
};



/* =================================================================================== */



static set<string_view> const TXL_SPECIAL {
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

  map<Label, NodePtr> referenceMap;

  NodePtr registerNode(Label const label, string_view const name) {
    auto node = graph->findNodeByName(name);
    if (!node) {
      node = graph->createNode(label, name);
      graph->__xmlOrderedNodes.push_back(node);
    }

    return referenceMap[label] = node;
  }

  [[nodiscard]]
  NodePtr findNodeByReference(Label const ref) {
    return referenceMap[ref];
  }

  bool VisitEnter(XMLElement const& element, XMLAttribute const *const firstAttr) override {
    string_view tagName = element.Name();
    Label label = LABEL_INCORRECT;
    Label ref = LABEL_INCORRECT;

    if (firstAttr && XMLUtil::StringEqual(firstAttr->Name(), "ref"))
      ref = firstAttr->IntValue();
    else
      for(auto attr = firstAttr; attr; attr = attr->Next() )
        if (XMLUtil::StringEqual(attr->Name(), "label")) {
          label = attr->IntValue();
          break;
        }

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
        WARNING("Desynchronization with expected label counter: expected " << expectedLabel << ", actual " << label);

      currentNode = is_reference ? findNodeByReference(ref) : registerNode(label, tagName);
      if (!currentNode) {
        ERROR("Undefined reference " << ref);
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
          root->connectTo(hangingNodes.top().second);

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

  void buildGraph(XMLDocument const& xmlDoc, TypeGraph *const graphToBuild) {
    reset(graphToBuild);

    xmlDoc.Accept(this);

    if (auto const count = hangingNodes.size(); count == 0)
      ERROR("Expected top node type to hang around");

    else if (count == 1 && hangingNodes.top().second->name == "program")
      hangingNodes.pop();

    else {
      ERROR("Something left behind!");
      while (!hangingNodes.empty()) {
        cerr << hangingNodes.top().second->name << endl;
        hangingNodes.pop();
      }
    }
  }

};



/* =================================================================================== */



static void renderAsDOT(TypeGraph const& g) {
  INFO("Drawing...");

  cout << "digraph G {" << endl;

  for (auto const type : g.__xmlOrderedNodes) {
    cout << "  <" << type->name << ">"
         << (type->name == "program" ? " [fillcolor=\"#FFA0A0\" style=filled]; <program>" : "")
         << " <" << type->name << ">"
         << " -> { ";

    int shouldBePrinted = static_cast<int>(type->references.size());
    for (auto const ref : type->references)
      cout << "<" << ref->name << ">" << (shouldBePrinted --> 1 ? ", " : "");

    cout << " }" << endl;
  }

  cout << "}" << endl;

  INFO("done");
}



/* =================================================================================== */



static void parse(XMLDocument &doc, const char* fileName, TypeGraph &graph) {
  INFO("Parsing...");

  if (auto result = doc.LoadFile(fileName); result != XML_SUCCESS) {
    ERROR("Loading failed: " << doc.ErrorIDToName(result));
    return;
  } else {
    INFO("XML loaded normaly");
  }

  TypeGraphBuilder builder;
  builder.buildGraph(doc, &graph);

  INFO("done");
}



/* =================================================================================== */



int main(/*int argc, char** argv*/) {
  TypeGraph graph;
  XMLDocument doc; // TODO: remove collapsing
  parse(doc, "java.xml", graph);

  INFO("parsing done");

  renderAsDOT(graph);

  return 0;
}
