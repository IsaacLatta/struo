#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "struo/parsing.hpp"

namespace {

using namespace struo;
using nlohmann::json;

std::optional<Json> get_child(const Json& parser, std::string_view key) {
    auto result = parser.toChild(key);
    EXPECT_TRUE(result);
    if(!result) {
        return std::nullopt;
    }
    EXPECT_TRUE(result.value());
    return std::move(result.value());
}

template<typename T>
std::optional<T> get_value(const Json& parser) {
    auto result = parser.getAs<T>();
    EXPECT_TRUE(result);
    if(!result) {
        return std::nullopt;
    }
    return std::move(result.value());
}

template<typename T>
std::optional<T> get_child_value(const Json& parser, std::string_view key) {
    auto child = get_child(parser, key);
    return child ? get_value<T>(*child) : std::nullopt;
}

TEST(JsonParser, TraversesScalars) {
    const Json parser{json::parse(R"({
        "name": "struo", "port": 8080, "enabled": true, "ratio": 1.25
    })")};
    EXPECT_EQ(get_child_value<std::string>(parser, "name"), "struo");
    EXPECT_EQ(get_child_value<int>(parser, "port"), 8080);
    EXPECT_EQ(get_child_value<bool>(parser, "enabled"), true);
    EXPECT_EQ(get_child_value<double>(parser, "ratio"), 1.25);
}

TEST(JsonParser, TraversesSequenceOfObjects) {
    const Json parser{json::parse(R"({"servers": [
        {"name": "first", "port": 1000},
        {"name": "second", "port": 2000}
    ]})")};
    auto servers = get_child(parser, "servers");
    ASSERT_TRUE(servers);
    auto elements = servers->getElements();
    ASSERT_TRUE(elements);
    ASSERT_EQ(elements.value().size(), 2u);
    EXPECT_EQ(get_child_value<std::string>(elements.value()[0], "name"), "first");
    EXPECT_EQ(get_child_value<int>(elements.value()[0], "port"), 1000);
    EXPECT_EQ(get_child_value<std::string>(elements.value()[1], "name"), "second");
    EXPECT_EQ(get_child_value<int>(elements.value()[1], "port"), 2000);
}

TEST(JsonParser, TraversesMapOfObjects) {
    const Json parser{json::parse(R"({"databases": {
        "primary": {"host": "primary.local", "port": 5432},
        "backup": {"host": "backup.local", "port": 5433}
    }})")};
    auto databases = get_child(parser, "databases");
    ASSERT_TRUE(databases);
    auto members = databases->getMembers();
    ASSERT_TRUE(members);
    ASSERT_EQ(members.value().size(), 2u);
    std::map<std::string, int> ports;
    for(const auto& [key_parser, value_parser] : members.value()) {
        const auto key = get_value<std::string>(key_parser);
        const auto port = get_child_value<int>(value_parser, "port");
        ASSERT_TRUE(key);
        ASSERT_TRUE(port);
        EXPECT_EQ(get_child_value<std::string>(value_parser, "host"), *key + ".local");
        ports.emplace(*key, *port);
    }
    EXPECT_EQ(ports, (std::map<std::string, int>{{"primary", 5432}, {"backup", 5433}}));
}

TEST(JsonParser, MissingChildReturnsEmptyOptional) {
    const Json parser{json::parse(R"({"name": "struo"})")};
    const auto result = parser.toChild("missing");
    ASSERT_TRUE(result);
    EXPECT_FALSE(result.value());
    const auto members = parser.getMembers();
    ASSERT_TRUE(members);
    EXPECT_EQ(members.value().size(), 1u);
}

TEST(JsonParser, NullChildIsPresentButNotScalar) {
    const Json parser{json::parse(R"({"value": null})")};
    auto child = get_child(parser, "value");
    ASSERT_TRUE(child);
    const auto result = child->getAs<int>();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
}

TEST(JsonParser, InvalidScalarConversionReturnsInvalidValue) {
    const Json parser{json("not-an-integer")};
    const auto result = parser.getAs<int>();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), INVALID_VALUE);
}

TEST(JsonParser, RejectsNonContainersDuringTraversal) {
    for(const auto& value : {json("hello"), json(42), json(true), json(nullptr)}) {
        const Json parser{value};
        const auto elements = parser.getElements();
        ASSERT_FALSE(elements);
        EXPECT_EQ(elements.error().code(), WRONG_TYPE);
        const auto members = parser.getMembers();
        ASSERT_FALSE(members);
        EXPECT_EQ(members.error().code(), WRONG_TYPE);
        const auto child = parser.toChild("missing");
        ASSERT_FALSE(child);
        EXPECT_EQ(child.error().code(), WRONG_TYPE);
    }
}

TEST(JsonParser, DistinguishesObjectsArraysAndScalars) {
    const Json array{json::parse("[1, 2, 3]")};
    const Json object{json::parse(R"({"foo": "bar"})")};
    EXPECT_EQ(array.getAs<int>().error().code(), WRONG_TYPE);
    EXPECT_EQ(object.getAs<std::string>().error().code(), WRONG_TYPE);
    EXPECT_EQ(array.getMembers().error().code(), WRONG_TYPE);
    EXPECT_EQ(array.toChild("foo").error().code(), WRONG_TYPE);
    EXPECT_EQ(object.getElements().error().code(), WRONG_TYPE);
}

TEST(JsonParser, AcceptsEmptyContainers) {
    const auto elements = Json{json::array()}.getElements();
    ASSERT_TRUE(elements);
    EXPECT_TRUE(elements.value().empty());
    const auto members = Json{json::object()}.getMembers();
    ASSERT_TRUE(members);
    EXPECT_TRUE(members.value().empty());
}

TEST(JsonParser, ChildrenOwnTheirValues) {
    auto child = get_child(Json{json::parse(R"({"value": "retained"})")}, "value");
    ASSERT_TRUE(child);
    EXPECT_EQ(get_value<std::string>(*child), "retained");
    auto members = Json{json::parse(R"({"key": "value"})")}.getMembers();
    ASSERT_TRUE(members);
    ASSERT_EQ(members.value().size(), 1u);
    EXPECT_EQ(get_value<std::string>(members.value()[0].first), "key");
    EXPECT_EQ(get_value<std::string>(members.value()[0].second), "value");
}

} // namespace
