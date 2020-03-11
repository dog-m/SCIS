#ifndef FRAGMENT_H
#define FRAGMENT_H

#include <string>
#include <vector>
#include <memory>
#include <ostream>

namespace scis {

  using namespace std;

  struct Fragment {
    struct Dependency {
      bool required = true;
      string target;
    };

    struct SourceBlock {
      virtual ~SourceBlock() = default;
      virtual void dump(ostream &str) = 0;
    };

    struct TextBlock : public SourceBlock {
      string text;

      void dump(ostream &str) override;
    };

    struct ParamReference : public SourceBlock {
      string id;

      void dump(ostream &str) override;
    };

    string language;
    string name;

    vector<Dependency> dependencies;
    vector<string> black_list;
    vector<string> params;
    vector<unique_ptr<SourceBlock>> source;

    void dump(ostream &str);
  }; // Fragment

} // scis

#endif // FRAGMENT_H
