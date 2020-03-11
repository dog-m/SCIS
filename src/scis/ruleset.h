#ifndef RULESET_H
#define RULESET_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

#include <ostream> // TODO: remove

namespace scis {

  using namespace std;

  struct FragmentRequest {
    string path;

    string getSource(string const& rootDir) const;
  };

  struct Pattern {
    bool somethingBefore = false;
    string text;
    bool somethingAfter = false;
  };

  struct Context {
    string id;

    virtual ~Context() = default;
    virtual void dump(ostream &str) = 0; // TODO: remove
  };

  struct BasicContext : public Context {
    struct Constraint {
      string id = "";
      string op;
      Pattern value;
    };

    vector<Constraint> constraints;

    void dump(ostream &str) override; // TODO: remove
  }; // BasicContext

  struct CompoundContext : public Context {
    struct Reference {
      bool negation = false;
      string id;
    };

    using Disjunction = vector<Reference>;
    using Conjunction = vector<Disjunction>;

    Conjunction references;
    void dump(ostream &str) override; // TODO: remove
  };

  struct Rule {
    struct Location {
      struct PathElement {
        string modifier;
        string statementId;
        optional<Pattern> pattern = nullopt;
      };

      string contextId;
      vector<PathElement> path;
      string pointcut;
    }; // Location

    struct Action {};

    struct MakeAction : public Action {
      struct Component {
        virtual ~Component() = default;
        virtual void dump(ostream &str) = 0; // TODO: remove
      };

      struct StringComponent : public Component {
        string text;

        void dump(ostream &str) override; // TODO: remove
      };

      struct ConstantComponent : public Component {
        string id;

        void dump(ostream &str) override; // TODO: remove
      };

      string target;
      vector<unique_ptr<Component>> components;
    }; // MakeAction

    struct AddAction : public Action {
      string fragmentId;
      vector<string> args;
    };

    struct Statement {
      Location location;

      vector<MakeAction> actionMake;
      vector<AddAction> actionAdd;
    };

    bool enabled = true;
    string id;

    vector<Statement> statements;
  }; // Rule

  struct Ruleset {
    vector<FragmentRequest> fragments;
    unordered_map<string_view, unique_ptr<Context>> contexts;
    unordered_map<string_view, unique_ptr<Rule>> rules;

    void dump(ostream &str);
  };

} // SCIS

#endif // RULESET_H
