#ifndef RULESET_H
#define RULESET_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

namespace SCIS {

  using namespace std;

  struct Fragment {
    string path;

    string getSource(string const& rootDir) const { return "?"; }
  };

  struct Context {
    string id;
  };

  struct BasicContext : public Context {
    struct Constraint {
      string id;
      string op;
      string value;
    };

    vector<Constraint> constraints;
  }; // BasicContext

  struct CompoundContext : public Context {
    vector<vector<string>> references;
  };

  struct Rule {
    struct Location {
      struct PathElement {
        string modifier;
        string statementId;
        optional<string> pattern = nullopt;
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
      struct FragmentUsageStatement {
        string fragmentId;
        vector<string> args;
      };

      vector<FragmentUsageStatement> fragments;
    }; // AddAction

    struct Stetement {
      Location location;

      optional<MakeAction> actionMake = nullopt;
      AddAction actionAdd;
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
