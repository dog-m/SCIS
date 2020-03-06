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

      virtual void toTXL(ostream &) const = 0;
    };

    struct PlainText final : public Literal {
      string text = "???";

      void toTXL(ostream &ss) const override;
    };

    struct TypeReference final : public Literal {
      optional<string> modifier = nullopt;
      string name = "???";
      optional<string> repeater = nullopt;

      void toTXL(ostream &ss) const override;
    };

    struct OptionalPlainText final : public Literal {
      string modifier = "???";
      string text = "???";

      void toTXL(ostream &ss) const override;
    };

    struct TypeVariant final {
      vector<unique_ptr<Literal>> pattern;

      void toTXL(ostream &ss, size_t const baseIndent) const;
    };

    struct Type final {
      string name;
      vector<TypeVariant> variants;

      void toTXL(ostream &ss, size_t const baseIndent) const;

      void toTXLDefinition(ostream &ss) const;
    };

    // end of sub-types

    unordered_map<string_view, unique_ptr<Type>> types;

    Type* findOrAddTypeByName(string_view const& name);

    void toDOT(ostream &ss) const;

  }; // TXLGrammar

}

#endif // TXLGRAMMAR_H
