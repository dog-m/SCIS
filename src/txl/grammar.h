#ifndef TXLGRAMMAR_H
#define TXLGRAMMAR_H

#include <vector>
#include <ostream>
#include <optional>
#include <memory>
#include <unordered_map>

namespace TXL {

  using namespace std;

  struct TXLGrammar {

    // sub-types

    struct Literal {
      virtual ~Literal() = default;

      virtual void toString(ostream &) const = 0;
    };

    struct PlainText: public Literal {
      string text = "???";

      void toString(ostream &ss) const override;
    };

    struct TypeReference : public Literal {
      string name = "???";
      optional<string> function = nullopt;

      void toString(ostream &ss) const override;
    };

    struct TypeVariant {
      vector<unique_ptr<Literal>> pattern;

      void toString(ostream &ss) const;
    };

    struct Type {
      string name;
      vector<TypeVariant> variants;

      void toString(ostream &ss) const;
    };

    // end of sub-types

    unordered_map<string_view, unique_ptr<Type>> types;

    Type* getTypeByName(string_view const& name);

  }; // TXLGrammar

}

#endif // TXLGRAMMAR_H
