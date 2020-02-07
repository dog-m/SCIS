#include <iostream>
#include "3rdparty/tinyxml2/tinyxml2.h"
#include <map>
#include <set>
#include <vector>

using namespace std;
using namespace tinyxml2;

using Label = int;

constexpr Label LABEL_INCORRECT = Label(-1);

struct Node {
  Label label;
  string_view type;
};



struct TypeGraphBuilder: public XMLVisitor {
  map<Label, Node*> stuff;
  Node* currentRoot = nullptr;

  bool VisitEnter(const XMLElement &element, const XMLAttribute *firstAttr) override {
    Label label = LABEL_INCORRECT;
    for(auto attr = firstAttr; attr; attr = attr->Next() )
      if (XMLUtil::StringEqual(attr->Name(), "label")) {
        XMLUtil::ToInt(attr->Value(), &label);
        break;
      }

    // пропускаем ссылки на уже пречисленные типы
    /*if (label == LABEL_INCORRECT)
      return true;*/

    string_view tagName = element.Name();

    /*// игнорируем специальные типы узлов
    if (tagName == "literal" || tagName == "id")
      return true;*/

    /*auto const underDashPos = tagName.find_first_of('_');
    if (underDashPos != string_view::npos) {
      auto const prefix = tagName.substr(0, underDashPos);
      auto const trueName = tagName.substr(underDashPos + 1);

      // игнорируем специальные типы узлов
      if (prefix == "repeat" || prefix == "opt" || prefix == "list" ||
          trueName == "literal" || trueName == "id")
        return true;
    }*/

    if (XMLUtil::StringEqual(firstAttr->Name(), "ref")) {
      Label ref;
      XMLUtil::ToInt(firstAttr->Value(), &ref);
      auto x = stuff[ref];
      if (x)
        cout << "->" << endl;
    } else {
      ;
    }

    //cout << "Found clean! " << tagName << endl;

    return true;
  }

  bool VisitExit(const XMLElement &) override {
    return true;
  }

  void buildGraph(XMLDocument const& doc) {
    currentRoot = nullptr;
    doc.Accept(this);
  }

  void showStuff() {
  }

};




static void parse(const char* fileName) {
  XMLDocument doc(true, COLLAPSE_WHITESPACE); // TODO: remove collapsing

  if (auto result = doc.LoadFile(fileName); result != XML_SUCCESS) {
    cout << "Loading failed: " << result << endl;
    return;
  }

  TypeGraphBuilder builder;
  builder.buildGraph(doc);

  builder.showStuff();

  //doc.SaveFile("1.xml", true);
}




int main(/*int argc, char** argv*/) {
  parse("java.xml");

  return 0;
}
