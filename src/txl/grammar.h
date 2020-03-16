#ifndef TXLGRAMMAR_H
#define TXLGRAMMAR_H

#include <vector>
#include <ostream>
#include <optional>
#include <memory>
#include <unordered_map>
#include <functional>

namespace txl {

  using namespace std;

  struct Grammar {

    // sub-types

    using NamingFunction = function<string(string_view const&)>;

    struct Literal {
      virtual ~Literal() = default;

      virtual void toTXL(ostream &, optional<NamingFunction> const namer = nullopt) const = 0;
    };

    struct PlainText final : public Literal {
      string text = "???";

      void toTXL(ostream &ss, optional<NamingFunction> const namer) const override;
    };

    struct TypeReference final : public Literal {
      optional<string> modifier = nullopt;
      string name = "???";
      optional<string> repeater = nullopt;

      void toTXL(ostream &ss, optional<NamingFunction> const namer) const override;
    };

    struct OptionalPlainText final : public Literal {
      string modifier = "???";
      string text = "???";

      void toTXL(ostream &ss, optional<NamingFunction> const namer) const override;
    };

    struct TypeVariant final {
      vector<unique_ptr<Literal>> pattern;

      void toTXL(ostream &ss, size_t const baseIndent) const;

      void toTXLWithNames(ostream &ss, NamingFunction const namer) const;
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

} // TXL

#endif // TXLGRAMMAR_H
