#ifndef RULESET_H
#define RULESET_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

namespace scis {

  using namespace std;

  struct Fragment {
    string path;

    string getSource(string const& rootDir) const { return "?"; }
  };

  struct Pattern {
    bool somethingBefore = false;
    string text;
    bool somethingAfter = false;
  };

  struct Context {
    string id;
  };

  struct BasicContext : public Context {
    struct Constraint {
      string id = "";
      string op;
      Pattern value;
    };

    vector<Constraint> constraints;
  }; // BasicContext

  struct CompoundContext : public Context {
    struct Reference {
      bool negation = false;
      string id;
    };

    using Disjunction = vector<Reference>;
    using Conjunction = vector<Disjunction>;

    Conjunction references;
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
      struct Component {};

      struct StringComponent : public Component {
        string text;
      };

      struct ConstantComponent : public Component {
        string id;
      };

      string target;
      vector<unique_ptr<Component>> components;
    }; // MakeAction

    struct AddAction : public Action {
      string fragmentId;
      vector<string> args;
    };

    struct Stetement {
      Location location;

      vector<MakeAction> actionMake;
      vector<AddAction> actionAdd;
    };

    bool enabled = true;
    string id;

    vector<Stetement> statements;
  }; // Rule

  struct Ruleset {
    vector<Fragment> fragments;
    unordered_map<string_view, unique_ptr<Context>> contexts;
    unordered_map<string_view, unique_ptr<Rule>> rules;
  };

} // SCIS

#endif // RULESET_H
