#include "struo/concepts.hpp"
#include "struo/parsing.hpp"

#include <gtest/gtest.h>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace parser_interface_test {
using namespace struo;

enum class Fault { None, MissingGetAs, WrongGetAs, MissingScalar, NonConst,
    MissingChild, WrongChild, MissingElements, WrongElements, MissingMembers, WrongMembers };

// Declarations alone suffice to test interface detection without instantiating bodies.
template<Fault F>
struct InterfaceParser {
    template<typename T>
    requires (F != Fault::MissingGetAs && F != Fault::NonConst &&
        (F != Fault::MissingScalar || !std::same_as<T, double>))
    std::conditional_t<F == Fault::WrongGetAs, T, Result<T>> getAs() const;

    template<typename T>
    requires (F == Fault::NonConst)
    Result<T> getAs();

    std::conditional_t<F == Fault::WrongChild, Result<InterfaceParser>,
        Result<std::optional<InterfaceParser>>> toChild(std::string_view) const
        requires (F != Fault::MissingChild);
    std::conditional_t<F == Fault::WrongElements, Result<std::vector<int>>,
        Result<std::vector<InterfaceParser>>> getElements() const
        requires (F != Fault::MissingElements);
    std::conditional_t<F == Fault::WrongMembers, Result<std::vector<InterfaceParser>>,
        Result<std::vector<std::pair<InterfaceParser, InterfaceParser>>>> getMembers() const
        requires (F != Fault::MissingMembers);
};

static_assert(HasParserInterface<Yaml>);
static_assert(HasParserInterface<Json>);
static_assert(HasParserInterface<Toml>);
static_assert(HasParserInterface<Yaml&>);
static_assert(HasParserInterface<const Json&>);
static_assert(HasParserInterface<Toml&&>);
static_assert(!HasParserInterface<volatile Json&>);
static_assert(!HasParserInterface<int>);
static_assert(!HasParserInterface<void>);
static_assert(HasParserInterface<InterfaceParser<Fault::None>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::MissingGetAs>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::WrongGetAs>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::MissingScalar>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::NonConst>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::MissingChild>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::WrongChild>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::MissingElements>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::WrongElements>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::MissingMembers>>);
static_assert(!HasParserInterface<InterfaceParser<Fault::WrongMembers>>);

struct Node {
    using Elements = std::vector<Node>;
    using Members = std::map<std::string, Node>;
    std::variant<int, std::string, Elements, Members> value;
};

// An independent in-memory adapter, with no dependency on a built-in parser.
class CustomParser {
public:
    explicit CustomParser(const Node& node) : node_(&node) {}

    template<typename T>
    Result<T> getAs() const {
        if constexpr (std::same_as<T, std::string>) {
            if (key_) return *key_;
            if (const auto* value = std::get_if<std::string>(&node_->value)) return *value;
        } else if constexpr (std::is_arithmetic_v<T>) {
            if (!key_) {
                if (const auto* value = std::get_if<int>(&node_->value)) return static_cast<T>(*value);
            }
        }
        return err(WRONG_TYPE, "expected scalar of requested type");
    }

    Result<std::optional<CustomParser>> toChild(std::string_view key) const {
        const auto* members = node_ ? std::get_if<Node::Members>(&node_->value) : nullptr;
        if (!members) return err(WRONG_TYPE, "expected object");
        if (const auto child = members->find(std::string{key}); child != members->end()) {
            return std::optional<CustomParser>{CustomParser{child->second}};
        }
        return std::optional<CustomParser>{};
    }

    Result<std::vector<CustomParser>> getElements() const {
        const auto* elements = node_ ? std::get_if<Node::Elements>(&node_->value) : nullptr;
        if (!elements) return err(WRONG_TYPE, "expected sequence");
        std::vector<CustomParser> result;
        for (const auto& element : *elements) result.emplace_back(element);
        return result;
    }

    Result<std::vector<std::pair<CustomParser, CustomParser>>> getMembers() const {
        const auto* members = node_ ? std::get_if<Node::Members>(&node_->value) : nullptr;
        if (!members) return err(WRONG_TYPE, "expected map");
        std::vector<std::pair<CustomParser, CustomParser>> result;
        for (const auto& [key, value] : *members) {
            result.emplace_back(CustomParser{key}, CustomParser{value});
        }
        return result;
    }

private:
    explicit CustomParser(const std::string& key) : node_(nullptr), key_(&key) {}
    const Node* node_;
    const std::string* key_{};
};

static_assert(HasParserInterface<CustomParser>);
static_assert(HasParserInterface<const CustomParser&>);

struct Child { int number{}; };
struct Config {
    int number{};
    std::vector<int> numbers;
    std::map<std::string, int> members;
    Child child;
    std::optional<int> missing;
};
} // namespace parser_interface_test

namespace struo {
template<>
struct SchemaTraits<parser_interface_test::Child> {
    static auto schema() {
        return Object{Field<&parser_interface_test::Child::number>{Keys{"number"}, REQUIRED}};
    }
};
template<>
struct SchemaTraits<parser_interface_test::Config> {
    static auto schema() {
        using parser_interface_test::Config;
        return Object{
            Field<&Config::number>{Keys{"number"}, REQUIRED},
            Field<&Config::numbers>{Keys{"numbers"}, REQUIRED},
            Field<&Config::members>{Keys{"members"}, REQUIRED},
            Field<&Config::child>{Keys{"child"}, REQUIRED},
            Field<&Config::missing>{Keys{"missing"}}
        };
    }
};
} // namespace struo

namespace parser_interface_test {
template<typename Parser>
concept CanLoad = requires(Parser&& parser) { load<Config>(std::forward<Parser>(parser)); };
static_assert(CanLoad<CustomParser>);
static_assert(CanLoad<CustomParser&>);
static_assert(CanLoad<const CustomParser&>);
static_assert(!CanLoad<int>);
static_assert(!CanLoad<InterfaceParser<Fault::MissingGetAs>>);
static_assert(!CanLoad<InterfaceParser<Fault::WrongChild>>);
static_assert(!CanLoad<InterfaceParser<Fault::WrongElements>>);
static_assert(!CanLoad<InterfaceParser<Fault::WrongMembers>>);

Node document() {
    return Node{Node::Members{
        {"number", Node{3}},
        {"numbers", Node{Node::Elements{Node{1}, Node{2}}}},
        {"members", Node{Node::Members{{"a", Node{4}}}}},
        {"child", Node{Node::Members{{"number", Node{5}}}}}
    }};
}

TEST(ParserInterface, LoadsAnIndependentCustomParser) {
    const auto node = document();
    CustomParser parser{node};
    const auto result = load<Config>(parser);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().number, 3);
    EXPECT_EQ(result.value().numbers, (std::vector<int>{1, 2}));
    EXPECT_EQ(result.value().members, (std::map<std::string, int>{{"a", 4}}));
    EXPECT_EQ(result.value().child.number, 5);
    EXPECT_FALSE(result.value().missing);
    EXPECT_TRUE(load<Config>(CustomParser{node}));
}

TEST(ParserInterface, AcceptsConstCustomAndBuiltInParsers) {
    const auto node = document();
    const CustomParser parser{node};
    EXPECT_TRUE(load<Config>(parser));
    const auto json = nlohmann::json::parse(R"({"number":3,"numbers":[1,2],"members":{"a":4},"child":{"number":5}})");
    const Json json_parser{json};
    EXPECT_TRUE(load<Config>(json_parser));
}

TEST(ParserInterface, PropagatesCustomParserErrorsWithTraversalPaths) {
    auto node = document();
    auto& members = std::get<Node::Members>(node.value);
    std::get<Node::Elements>(members.at("numbers").value)[1].value = std::string{"bad"};
    const auto result = load<Config>(CustomParser{node});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
    EXPECT_TRUE(result.error().what().starts_with("numbers[1]: ")) << result.error().what();
}

TEST(ParserInterface, PropagatesCustomNavigationErrors) {
    const Node node{42};
    const auto result = load<Config>(CustomParser{node});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
    EXPECT_TRUE(result.error().what().starts_with("number: ")) << result.error().what();
}
} // namespace parser_interface_test
