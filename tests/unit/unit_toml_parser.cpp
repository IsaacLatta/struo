#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "struo/parsing/TomlParser.hpp"
#include "struo/struo.hpp"

namespace {

using namespace struo;

TomlParser scalar(toml::table& source, std::string_view text) {
    source = toml::parse("value = " + std::string{text});
    return TomlParser{*source.get("value")};
}

template<typename T>
std::optional<T> child_value(const TomlParser& parser, std::string_view key) {
    auto child = parser.toChild(key);
    EXPECT_TRUE(child);
    if(!child) return std::nullopt;
    EXPECT_TRUE(child.value());
    if(!child.value()) return std::nullopt;
    auto value = child.value()->template getAs<T>();
    EXPECT_TRUE(value);
    if(!value) return std::nullopt;
    return std::move(value.value());
}

TEST(TomlParser, TraversesScalarsAndLiteralKeys) {
    const auto source = toml::parse(R"(
        name = "struo"
        port = 8080
        enabled = true
        ratio = 1.25
        "literal.key" = 7
    )");
    const TomlParser parser{source};
    EXPECT_EQ(child_value<std::string>(parser, "name"), "struo");
    EXPECT_EQ(child_value<int>(parser, "port"), 8080);
    EXPECT_EQ(child_value<bool>(parser, "enabled"), true);
    EXPECT_EQ(child_value<double>(parser, "ratio"), 1.25);
    EXPECT_EQ(child_value<int>(parser, "literal.key"), 7);
    const auto missing = parser.toChild("missing");
    ASSERT_TRUE(missing);
    EXPECT_FALSE(missing.value());
}

TEST(TomlParser, TraversesArraysOfTablesAndNestedTables) {
    const auto source = toml::parse(R"(
        [[servers]]
        name = "first"
        [[servers]]
        name = "second"
        [databases.primary]
        port = 5432
        [databases.backup]
        port = 5433
    )");
    const TomlParser parser{source};
    auto servers = parser.toChild("servers");
    ASSERT_TRUE(servers);
    ASSERT_TRUE(servers.value());
    auto elements = servers.value()->getElements();
    ASSERT_TRUE(elements);
    ASSERT_EQ(elements.value().size(), 2u);
    EXPECT_EQ(child_value<std::string>(elements.value()[0], "name"), "first");
    EXPECT_EQ(child_value<std::string>(elements.value()[1], "name"), "second");
    auto databases = parser.toChild("databases");
    ASSERT_TRUE(databases);
    ASSERT_TRUE(databases.value());
    auto members = databases.value()->getMembers();
    ASSERT_TRUE(members);
    std::map<std::string, int> ports;
    for(const auto& [key_parser, value_parser] : members.value()) {
        auto key = key_parser.getAs<std::string>();
        auto port = child_value<int>(value_parser, "port");
        ASSERT_TRUE(key);
        ASSERT_TRUE(port);
        ports.emplace(key.value(), *port);
    }
    EXPECT_EQ(ports, (std::map<std::string, int>{{"primary", 5432}, {"backup", 5433}}));
}

TEST(TomlParser, RejectsInvalidConversionsAndShapes) {
    toml::table source;
    const auto invalid = scalar(source, R"("not-an-integer")").getAs<int>();
    ASSERT_FALSE(invalid);
    EXPECT_EQ(invalid.error().code(), INVALID_VALUE);
    const auto overflow = scalar(source, "256").getAs<std::uint8_t>();
    ASSERT_FALSE(overflow);
    EXPECT_EQ(overflow.error().code(), INVALID_VALUE);
    const auto value = scalar(source, "42");
    EXPECT_EQ(value.getElements().error().code(), WRONG_TYPE);
    EXPECT_EQ(value.getMembers().error().code(), WRONG_TYPE);
    EXPECT_EQ(value.toChild("missing").error().code(), WRONG_TYPE);
    EXPECT_EQ(scalar(source, "[]").getAs<int>().error().code(), WRONG_TYPE);
    EXPECT_EQ(scalar(source, "{}").getAs<int>().error().code(), WRONG_TYPE);
    EXPECT_EQ(scalar(source, "[]").getMembers().error().code(), WRONG_TYPE);
    EXPECT_EQ(scalar(source, "{}").getElements().error().code(), WRONG_TYPE);
    // TOML date values remain typed values, not implicit strings or numbers.
    EXPECT_EQ(scalar(source, "2026-09-30").getAs<int>().error().code(), INVALID_VALUE);
}

TEST(TomlParser, BorrowsNodesAndKeysFromTheLiveDocument) {
    const auto source = toml::parse(R"(
        nested = {answer = "borrowed"}
        items = [{answer = 7}]
        "" = 1
    )");
    // Child views may outlive the parser wrapper, but the document stays alive.
    auto child = TomlParser{source}.toChild("nested");
    ASSERT_TRUE(child);
    ASSERT_TRUE(child.value());
    auto copied = *child.value();
    child.value().reset();
    auto moved = std::move(copied);
    EXPECT_EQ(child_value<std::string>(moved, "answer"), "borrowed");
    auto answer = moved.toChild("answer");
    ASSERT_TRUE(answer);
    ASSERT_TRUE(answer.value());
    const auto text = answer.value()->getAs<std::string_view>();
    ASSERT_TRUE(text);
    EXPECT_EQ(text.value().data(), source["nested"]["answer"].value<std::string_view>()->data());

    auto members = TomlParser{source}.getMembers();
    ASSERT_TRUE(members);
    ASSERT_EQ(members.value().size(), 3u);
    auto key = std::move(members.value()[0].first);
    members.value().clear();
    EXPECT_EQ(key.getAs<std::string>().value(), "");
    EXPECT_EQ(key.getAs<std::filesystem::path>().value(), std::filesystem::path{});
    EXPECT_EQ(key.getAs<int>().error().code(), INVALID_VALUE);
    EXPECT_EQ(key.toChild("missing").error().code(), WRONG_TYPE);
    EXPECT_EQ(key.getMembers().error().code(), WRONG_TYPE);
    EXPECT_EQ(key.getElements().error().code(), WRONG_TYPE);

    auto items = TomlParser{source}.toChild("items");
    ASSERT_TRUE(items);
    ASSERT_TRUE(items.value());
    auto elements = items.value()->getElements();
    ASSERT_TRUE(elements);
    ASSERT_EQ(elements.value().size(), 1u);
    EXPECT_EQ(child_value<int>(elements.value()[0], "answer"), 7);
    const toml::value<std::string> standalone_value{"borrowed"};
    const TomlParser standalone{standalone_value};
    EXPECT_EQ(standalone.getAs<std::string>().value(), "borrowed");
}

TEST(TomlParser, AcceptsEmptyContainers) {
    toml::table source;
    const auto elements = scalar(source, "[]").getElements();
    ASSERT_TRUE(elements);
    EXPECT_TRUE(elements.value().empty());
    const toml::table empty;
    const auto members = TomlParser{empty}.getMembers();
    ASSERT_TRUE(members);
    EXPECT_TRUE(members.value().empty());
}

struct TomlUse { std::string target; };
using TomlChoice = std::variant<int, TomlUse>;
enum class TomlKind { NUMBER, USE };
struct TomlConfig {
    std::string name;
    TomlChoice choice;
    std::map<std::string, int> targets;
};

} // namespace

namespace struo {

template<>
struct SchemaTraits<TomlUse> {
    static auto schema() {
        return Object{Field<&TomlUse::target>{Keys{"target"}}};
    }
};

template<>
struct SchemaTraits<TomlChoice> {
    static auto schema() {
        return Variant{Bindings{Bind<TomlKind::USE, TomlUse>{}, Bind<TomlKind::NUMBER, int>{}}};
    }
};

template<>
struct SchemaTraits<TomlConfig> {
    static auto schema() {
        return Object{
            Field<&TomlConfig::name>{Keys{"name", "label"}, Defaults{[] { return "default"; }}},
            Field<&TomlConfig::choice>{Keys{"choice"}},
            Field<&TomlConfig::targets>{Keys{"targets"}}
        };
    }
};

} // namespace struo

namespace {

TEST(TomlParser, LoadsVariantsAliasesAndDefaults) {
    const auto result = load<TomlConfig>(TomlParser{toml::parse(R"(
        label = "struo"
        choice = {type = "USE", value = {target = "front"}}
        targets = {front = 1}
    )")});
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().name, "struo");
    EXPECT_EQ(std::get<TomlUse>(result.value().choice).target, "front");
    const auto defaults = load<TomlConfig>(TomlParser{toml::parse("")});
    ASSERT_TRUE(defaults);
    EXPECT_EQ(defaults.value().name, "default");
}

TEST(TomlParser, ReportsTypeErrorsWithVariantPaths) {
    const auto result = load<TomlConfig>(TomlParser{toml::parse(
        R"(choice = {type = "USE", value = {target = []}})")});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
    EXPECT_TRUE(result.error().what().starts_with("choice<USE>.value.target: ")) << result.error().what();
}

} // namespace
