#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <ostream>
#include <functional>

namespace scis {

  using namespace std;

  inline string const ANNOTATION_GROUP_SEPARATOR = ":";
  inline string const ANNOTATION_POI_GROUP_PREFIX = "poi" + ANNOTATION_GROUP_SEPARATOR;

  inline string const DEFAULT_PIPELINE = R"***(txl "%SRC%" "%TRANSFORM%" -o "%DST%" %PARAMS%)***";

  inline string const ANNOTATION_TEMPLATE_MATCH   = "match";
  inline string const ANNOTATION_TEMPLATE_REPLACE = "replace";

  struct GrammarAnnotation final {

    struct Pointcut final {
      struct Step final {
        string function;
        unordered_map<string, string> args;
      }; // Step

      string name;
      vector<Step> aglorithm;
      bool caretViolation = false;
    }; // Pointcut

    struct Template final {
      using NamingFunction = function<string(string const&)>;

      struct Block {
        virtual ~Block() = default;
        virtual void dump(ostream &str) const = 0;
        virtual void toTXLWithNames(ostream &str,
                           NamingFunction const& namer) const = 0;
      }; // Block

      struct TextBlock final : public Block {
        string text;

        void dump(ostream &str) const override;
        void toTXLWithNames(ostream &str,
                            NamingFunction const& namer) const override;
      }; // TextBlock

      struct PointcutLocation final : public Block {
        string name;

        void dump(ostream &str) const override;
        void toTXLWithNames(ostream &str,
                            NamingFunction const& namer) const override;
      }; // PointcutLocation

      struct TypeReference final : public Block {
        string typeId;

        void dump(ostream &str) const override;
        void toTXLWithNames(ostream &str,
                            NamingFunction const& namer) const override;
      }; // TypeReference

      string kind;
      vector<unique_ptr<Block>> blocks;
      bool generateFromGrammar = false;

      void toTXLWithNames(ostream &str,
                          NamingFunction const& namer) const;
    }; // Template

    struct DirectedAcyclicGraph final {
      struct Keyword final {
        string id;
        string type;
        string searchType;
        bool sequential = false;
        optional<string> filterPOI = nullopt;
        unordered_map<string, unique_ptr<Pointcut>> pointcuts;
        unordered_map<string, unique_ptr<Template>> templates;
        vector<string> subnodes;

        Template* addTemplate(string const& kind);
        Pointcut* addPointcut(string const& name);

        Template const* getTemplate(string const& kind) const;
      }; // Keyword

      vector<Keyword const*> topKeywords;
      unordered_map<string, unique_ptr<Keyword>> keywords;
    }; // DAG

    struct GrammarDescription final {
      string language;
      string txlSourceFilename;
      DirectedAcyclicGraph graph;
      string userVariableType = "stringlit";
    }; // GrammarDescription

    enum class FunctionPolicy {
      UNSPECIFIED = 0,
      DIRECT_CALL,
      BEFORE_ALL,
      AFTER_ALL,
    }; // FunctionPolicy

    struct Function final {
      struct Parameter final {
        string id;
        string type;
      }; // Parameter

      string name;
      bool isRule = false;
      FunctionPolicy callPolicy = FunctionPolicy::UNSPECIFIED;
      vector<Parameter> params;
      string source;
    }; // Function

    struct PointOfInterest final {
      string id;
      string keyword;
      vector<string> valueTypePath;
    }; // POI

    GrammarDescription grammar;
    vector<unique_ptr<Function>> library;
    unordered_map<string, unique_ptr<PointOfInterest>> pointsOfInterest;

    string pipeline = DEFAULT_PIPELINE;

    void dump(ostream &str);
  }; // GrammarAnnotation

} // scis

#endif // ANNOTATION_H
