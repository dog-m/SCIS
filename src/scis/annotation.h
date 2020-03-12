#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace scis {

  using namespace std;

  struct GrammarAnnotation {

    struct Pointcut {
      struct Step {
        string function;
        unordered_map<string, string> args; // TODO: get better idea
      };

      string name;
      string refType;
      string refAlias;
      vector<Step> aglorithm;
    }; // Pointcut

    struct Pattern {
      struct Block {};

      struct TextBlock : public Block {
        string text;
      };

      struct PointcutLocation : public Block {
        string name;
      };

      string searchType;
      vector<unique_ptr<Block>> blocks;
    }; // Pattern

    struct DirectedAcyclicGraph {
      struct Keyword {
        string id;
        string type;
        unordered_map<string_view, unique_ptr<Pointcut>> pointcuts;
        vector<Pattern> replacement_patterns;
        vector<string> subnodes;
      };

      unordered_map<string_view, unique_ptr<Keyword>> keywords;
    }; // DAG

    struct GrammarDescription {
      string language;
      string txlSourceFilename;
      DirectedAcyclicGraph graph;
    };

    enum class FunctionPolicy {
      DIRECT_CALL,
      BEFORE_ALL,
      AFTER_ALL,
    };

    struct Function {
      struct Parameter {
        string id;
        string type;
      };

      string name;
      FunctionPolicy callPolicy = FunctionPolicy::DIRECT_CALL;
      vector<Parameter> params;
      string source;
    }; // Function

    struct PointOfInterest {
      string id;
      string keyword;
      vector<string> valueTypePath;
    };

    GrammarDescription grammar;
    vector<unique_ptr<Function>> library;
    unordered_map<string_view, unique_ptr<PointOfInterest>> pointsOfInterest;
  }; // GrammarAnnotation

} // scis

#endif // ANNOTATION_H
