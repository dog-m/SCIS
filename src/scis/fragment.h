#ifndef FRAGMENT_H
#define FRAGMENT_H

#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include <unordered_map>

namespace scis {

  using namespace std;

  struct Fragment {
    struct Dependency {
      bool required = true;
      string target;
    };

    struct SourceBlock {
      virtual ~SourceBlock() = default;
      virtual void dump(ostream &str) const = 0;

      virtual void toTXL(ostream &str,
                         unordered_map<string_view, string_view> const& param2arg) const = 0;
    };

    struct TextBlock : public SourceBlock {
      string text;

      void dump(ostream &str) const override;

      void toTXL(ostream &str,
                 unordered_map<string_view, string_view> const& param2arg) const override;
    };

    struct ParamReference : public SourceBlock {
      string id;

      void dump(ostream &str) const override;

      void toTXL(ostream &str,
                 unordered_map<string_view, string_view> const& param2arg) const override;
    };

    string language;
    string name;

    vector<Dependency> dependencies;
    vector<string> black_list;
    vector<string> params;
    vector<unique_ptr<SourceBlock>> source;

    void dump(ostream &str) const;

    void toTXL(ostream &str, const vector<string>& args) const;
  }; // Fragment

} // scis

#endif // FRAGMENT_H
