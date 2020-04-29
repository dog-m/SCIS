#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <ostream>

namespace scis {

  using namespace std;

  inline string const POI_GROUP_PREFIX = "poi:";

  struct GrammarAnnotation final {

    struct Pointcut final {

      struct Step final {
        string function;
        unordered_map<string, string> args; // TODO: get better idea
      };

      string name;
      string fragType;
      string fragAlias;
      vector<Step> aglorithm;
    }; // Pointcut

    struct Pattern final {
      struct Block {
        virtual ~Block() = default;
        virtual void dump(ostream &str) = 0;
      };

      struct TextBlock final : public Block {
        string text;

        void dump(ostream &str) override;
      };

      struct PointcutLocation final : public Block {
        string name;

        void dump(ostream &str) override;
      };

      struct TypeReference final : public Block {
        string typeId;

        void dump(ostream &str) override;
      };

      string searchType;
      vector<unique_ptr<Block>> blocks;
    }; // Pattern

    struct DirectedAcyclicGraph final {
      struct Keyword final {
        string id;
        string type;
        bool sequential = true;
        optional<string> filterPOI = nullopt;
        unordered_map<string, unique_ptr<Pointcut>> pointcuts;
        vector<Pattern> replacement_patterns;
        vector<string> subnodes;
      };

      vector<Keyword const*> topKeywords;
      unordered_map<string, unique_ptr<Keyword>> keywords;
    }; // DAG

    struct GrammarDescription final {
      string language;
      string txlSourceFilename;
      DirectedAcyclicGraph graph;
      string baseSequenceType;
    };

    enum class FunctionPolicy {
      DIRECT_CALL,
      BEFORE_ALL,
      AFTER_ALL,
    };

    struct Function final {
      struct Parameter final {
        string id;
        string type;
      };

      string name;
      bool isRule = false;
      FunctionPolicy callPolicy = FunctionPolicy::DIRECT_CALL;
      vector<Parameter> params;
      string source;
    }; // Function

    struct PointOfInterest final {
      string id;
      string keyword;
      vector<string> valueTypePath;
    };

    GrammarDescription grammar;
    vector<unique_ptr<Function>> library;
    unordered_map<string, unique_ptr<PointOfInterest>> pointsOfInterest;

    string pipeline = "txl %SRC% %TRANSFORM% %PARAMS%";

    void dump(ostream &str);
  }; // GrammarAnnotation

} // scis

#endif // ANNOTATION_H
