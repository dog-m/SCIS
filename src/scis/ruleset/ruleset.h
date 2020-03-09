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
    struct Constraint {};

    string id;
    vector<vector<Constraint>> constraints;
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
    };

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
    };

    struct AddAction : public Action {
      struct FragmentUsageStatement {
        string fragmentId;
        vector<string> args;
      };

      vector<FragmentUsageStatement> fragments;
    };

    struct Stetement {
      Location location;
      vector<unique_ptr<Action>> actions;
    };

    bool enabled = true;
    string id;

    vector<Stetement> statements;
  };

  struct Ruleset {
    vector<Fragment> fragments;
    unordered_map<string_view, Context> contexts;
    unordered_map<string_view, Rule> rules;
  };

}

#endif // RULESET_H
