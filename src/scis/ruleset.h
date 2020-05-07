#ifndef RULESET_H
#define RULESET_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

#include <ostream>

namespace scis {

  using namespace std;

  struct SourceInfo {
    int32_t declarationLine = 0;
  };

  struct FragmentRequest final : public SourceInfo {
    string path;
  };

  struct PatternFragment final : public SourceInfo {
    bool somethingBefore = false;
    string text;
    bool somethingAfter = false;
  };
  // string like "*text*"
  using Pattern = vector<PatternFragment>;

  struct Context : public SourceInfo {
    string id;

    virtual ~Context() = default;
    virtual void dump(ostream &str) = 0; // TODO: remove
  };

  struct BasicContext final : public Context {
    struct Constraint {
      string id = "";
      string op;
      Pattern value;
    };

    vector<Constraint> constraints;

    void dump(ostream &str) override; // TODO: remove
  }; // BasicContext

  inline string const GLOBAL_CONTEXT_ID = "@";
  inline unique_ptr<BasicContext> const GLOBAL_CONTEXT = make_unique<BasicContext>();

  struct CompoundContext final : public Context {
    struct Reference {
      bool isNegative = false;
      string id;
    };

    using Disjunction = vector<Reference>;
    using Conjunction = vector<Disjunction>;

    Conjunction references;
    void dump(ostream &str) override; // TODO: remove
  }; // CompoundContext

  struct Rule final : public SourceInfo {
    struct Location final {
      struct PathElement final {
        string modifier;
        string keywordId;
        optional<Pattern> pattern = nullopt;
      };

      string contextId;
      vector<PathElement> path;
      string pointcut;
    }; // Location

    struct Action : public SourceInfo {};

    struct MakeAction final : public Action {
      struct Component {
        virtual ~Component() = default;
        virtual void dump(ostream &str) const = 0; // TODO: remove
        virtual void toTXL(ostream &str) const = 0;
      };

      struct StringComponent final : public Component {
        string text;

        void dump(ostream &str) const override; // TODO: remove
        void toTXL(ostream &str) const override;
      };

      struct ConstantComponent final : public Component {
        string group;
        string id;

        void dump(ostream &str) const override; // TODO: remove
        void toTXL(ostream &str) const override;
      };

      string target;
      vector<unique_ptr<Component>> components;
    }; // MakeAction

    struct AddAction final : public Action {
      string fragmentId;
      vector<string> args;
    };

    struct Statement final : public SourceInfo {
      Location location;

      vector<MakeAction> actionMake;
      vector<AddAction> actionAdd;
    };

    bool enabled = true;
    string id;

    vector<Statement> statements;
  }; // Rule

  struct Ruleset final {
    vector<FragmentRequest> fragments;
    unordered_map<string, unique_ptr<Context>> contexts;
    unordered_map<string, unique_ptr<Rule>> rules;

    void dump(ostream &str);
  };

} // SCIS

#endif // RULESET_H
