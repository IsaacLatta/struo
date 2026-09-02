#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <string_view>

#include "struo/parsing/YamlParser.hpp"

namespace {

using namespace struo;

std::optional<YamlParser> get_child(const YamlParser& parser, std::string_view key) {
    auto result = parser.toChild(key);

    EXPECT_TRUE(result);
    if (!result) {
        return std::nullopt;
    }

    EXPECT_TRUE(result.value());
    if (!result.value()) {
        return std::nullopt;
    }

    return std::move(*result.value());
}

template<typename T>
std::optional<T> get_value(const YamlParser& parser) {
    auto result = parser.getAs<T>();

    EXPECT_TRUE(result);
    if (!result) {
        return std::nullopt;
    }

    return std::move(result.value());
}

template<typename T>
std::optional<T> get_child_value(const YamlParser& parser, std::string_view key) {
    auto child = get_child(parser, key);
    if (!child) {
        return std::nullopt;
    }

    return get_value<T>(*child);
}

TEST(YamlParser, TraversesScalars) {
    const YamlParser parser{
        YAML::Load(R"(
name: struo
port: 8080
enabled: true
ratio: 1.25
)")
    };

    auto name = get_child_value<std::string>(parser, "name");
    ASSERT_TRUE(name);
    EXPECT_EQ(*name, "struo");

    auto port = get_child_value<int>(parser, "port");
    ASSERT_TRUE(port);
    EXPECT_EQ(*port, 8080);

    auto enabled = get_child_value<bool>(parser, "enabled");
    ASSERT_TRUE(enabled);
    EXPECT_TRUE(*enabled);

    auto ratio = get_child_value<double>(parser, "ratio");
    ASSERT_TRUE(ratio);
    EXPECT_DOUBLE_EQ(*ratio, 1.25);
}

TEST(YamlParser, TraversesSequenceOfObjects) {
    const YamlParser parser{
        YAML::Load(R"(
servers:
  - name: first
    port: 1000
  - name: second
    port: 2000
)")
    };

    auto servers = get_child(parser, "servers");
    ASSERT_TRUE(servers);

    auto elements = servers->getElements();
    ASSERT_TRUE(elements);
    ASSERT_EQ(elements.value().size(), 2);

    auto first_name = get_child_value<std::string>(elements.value()[0], "name");
    ASSERT_TRUE(first_name);
    EXPECT_EQ(*first_name, "first");

    auto first_port = get_child_value<int>(elements.value()[0], "port");
    ASSERT_TRUE(first_port);
    EXPECT_EQ(*first_port, 1000);

    auto second_name = get_child_value<std::string>(elements.value()[1], "name");
    ASSERT_TRUE(second_name);
    EXPECT_EQ(*second_name, "second");

    auto second_port = get_child_value<int>(elements.value()[1], "port");
    ASSERT_TRUE(second_port);
    EXPECT_EQ(*second_port, 2000);
}

TEST(YamlParser, TraversesMapOfObjects) {
    const YamlParser parser{
        YAML::Load(R"(
databases:
  primary:
    host: primary.local
    port: 5432
  backup:
    host: backup.local
    port: 5433
)")
    };

    auto databases = get_child(parser, "databases");
    ASSERT_TRUE(databases);

    auto members = databases->getMembers();
    ASSERT_TRUE(members);
    ASSERT_EQ(members.value().size(), 2);

    bool found_primary = false;
    bool found_backup = false;

    for (const auto& [key_parser, value_parser] : members.value()) {
        auto key = get_value<std::string>(key_parser);
        ASSERT_TRUE(key);

        auto host = get_child_value<std::string>(value_parser, "host");
        ASSERT_TRUE(host);

        auto port = get_child_value<int>(value_parser, "port");
        ASSERT_TRUE(port);

        if (*key == "primary") {
            found_primary = true;
            EXPECT_EQ(*host, "primary.local");
            EXPECT_EQ(*port, 5432);
        }
        else if (*key == "backup") {
            found_backup = true;
            EXPECT_EQ(*host, "backup.local");
            EXPECT_EQ(*port, 5433);
        }
        else {
            FAIL() << "unexpected key: " << *key;
        }
    }

    EXPECT_TRUE(found_primary);
    EXPECT_TRUE(found_backup);
}

TEST(YamlParser, MissingChildReturnsEmptyOptional) {
    const YamlParser parser{
        YAML::Load("name: struo")
    };

    auto result = parser.toChild("missing");

    ASSERT_TRUE(result);
    EXPECT_FALSE(result.value());
}

TEST(YamlParser, InvalidScalarConversionReturnsInvalidValue) {
    const YamlParser parser { YAML::Load("not-an-integer") };

    auto result = parser.getAs<int>();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), INVALID_VALUE);
}

TEST(YamlParser, ScalarIsNotSequence) {
    const YamlParser parser { YAML::Load("hello") };

    auto result = parser.getElements();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
}

TEST(YamlParser, ScalarIsNotMap) {
    const YamlParser parser { YAML::Load("hello") };

    auto result = parser.getMembers();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
}

TEST(YamlParser, SequenceIsNotScalar) {
    const YamlParser parser { YAML::Load("[1, 2, 3]") };

    auto result = parser.getAs<int>();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
}

TEST(YamlParser, MapIsNotScalar) {
    const YamlParser parser { YAML::Load("{foo: bar}") };

    auto result = parser.getAs<std::string>();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
}

}