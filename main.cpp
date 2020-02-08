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
  inline pool_item getNew(Args&&... args) {
    auto const item = new T(forward<Args>(args)...);
    return items.emplace_back(item);
  }

  template<typename U, typename ...Args>
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
  string_view type;

  TypeNode(Label const label, string_view const type):
    label(label), type(type) {}
};

struct TypeGraph {
  auto_pool<TypeNode> pool;

  map<Label, decltype(pool)::pool_item> types;

  void createNode(Label const label, string_view const type) {
    types[label] = pool.getNew(label, type);
  }

  decltype(pool)::pool_item findNodeByLabel(Label const label) {
    return types[label];
  }
};



/* =================================================================================== */



struct TypeGraphBuilder: public XMLVisitor {
  TypeGraph* graph;
  TypeNode* currentRoot = nullptr;
  stack<TypeNode*> passedNodes;
  Label expectedLabel = 0;

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

    static set<string_view> const SPECIAL {
      "NL", "IN", "EX", "SPON", "SPOFF",
      "empty", "id", "number", "charlit", "stringlit"
    };
    bool const is_special = (SPECIAL.find(tagName) != SPECIAL.cend());

    if (is_special)
      return false;

    if (!is_reference)
      ++expectedLabel;

    if (!is_labeled && !is_reference && !is_special) {
      label = expectedLabel;
      WARNING("Unindexed node type: " << tagName << ". It probably have label = " << label);
      is_labeled = true;
    }

    if (is_labeled && label != expectedLabel)
      WARNING("Desynchronization with expected label counter: expected " << expectedLabel << ", actual " << label);

    if (is_reference) {
      Label ref = firstAttr->IntValue();
      auto const node = graph->findNodeByLabel(ref);
      if (node)
        INFO(" -> " << node->type);
      else
        WARNING("Undefined reference " << ref);
    }
    else {
      //INFO("Found clean! " << tagName);
      graph->createNode(label, tagName);
    }

    return true;
  }

  bool VisitExit(XMLElement const& element) override {
    if (!passedNodes.empty())
      passedNodes.pop();

    return true;
  }

  void buildGraph(XMLDocument const& doc, TypeGraph *const graphToBuild) {
    currentRoot = nullptr;
    expectedLabel = 0;
    graph = graphToBuild;
    doc.Accept(this);
  }

};



/* =================================================================================== */



static void parse(const char* fileName) {
  XMLDocument doc(true, COLLAPSE_WHITESPACE); // TODO: remove collapsing

  if (auto result = doc.LoadFile(fileName); result != XML_SUCCESS) {
    ERROR("Loading failed: " << doc.ErrorIDToName(result));
    return;
  } else {
    INFO("XML loaded normaly");
  }

  TypeGraph graph;

  TypeGraphBuilder builder;
  builder.buildGraph(doc, &graph);

  //doc.SaveFile("1.xml", true);
}



/* =================================================================================== */



int main(/*int argc, char** argv*/) {
  parse("java.xml");

  return 0;
}
