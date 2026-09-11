#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "struo/struo.hpp"

namespace {

struct Item {
    int foo{};
};

struct DefaultItem {
    int foo{};
};

struct Tree {
    Item object;
    std::vector<Item> array;
    std::map<std::string, Item> map;
    std::map<int, Item> numeric_map;
    std::vector<std::map<std::string, std::vector<Item>>> nested;
    DefaultItem defaults;
};

constexpr auto Positive = [](int value) -> struo::Result<void> {
    if(value > 0) {
        return struo::ok();
    }
    return struo::err(struo::INVALID_VALUE, "expected positive");
};

} // namespace

namespace struo {

template<>
struct SchemaTraits<Item> {
    static auto schema() {
        return Object{
            Field<&Item::foo>{Keys{"foo", "f"}, REQUIRED, Constraints{Positive}}
        };
    }
};

template<>
struct SchemaTraits<DefaultItem> {
    static auto schema() {
        return Object{
            Field<&DefaultItem::foo>{Keys{"foo"}, Defaults{[]() -> Result<std::optional<int>> {
                return err(INVALID_VALUE, "default unavailable");
            }}}
        };
    }
};

template<>
struct SchemaTraits<Tree> {
    static auto schema() {
        return Object{
            Field<&Tree::object>{Keys{"object", "my.object"}},
            Field<&Tree::array>{Keys{"array"}},
            Field<&Tree::map>{Keys{"map"}},
            Field<&Tree::numeric_map>{Keys{"numeric_map"}},
            Field<&Tree::nested>{Keys{"nested"}},
            Field<&Tree::defaults>{Keys{"defaults"}}
        };
    }
};

} // namespace struo

namespace {

using namespace struo;

void expect_error_at(std::string_view yaml, std::string_view path,
                     ErrorCode code = INVALID_VALUE) {
    const auto result = load<Tree>(YamlParser{YAML::Load(std::string{yaml})});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), code);
    EXPECT_TRUE(result.error().what().starts_with(std::string{path} + ": "))
        << result.error().what();
}

TEST(Traversal, TracksObjectFieldDuringParsingAndValidation) {
    expect_error_at("object: {foo: nope}", "object.foo");
    expect_error_at("object: {foo: -1}", "object.foo");
}

TEST(Traversal, TracksArrayOfObjectsDuringParsingAndValidation) {
    expect_error_at("array: [{foo: 1}, {foo: nope}]", "array[1].foo");
    expect_error_at("array: [{foo: 1}, {foo: -1}]", "array[1].foo");
}

TEST(Traversal, TracksStringMapOfObjectsDuringParsingAndValidation) {
    expect_error_at("map: {first: {foo: 1}, second: {foo: nope}}", "map[\"second\"].foo");
    expect_error_at("map: {first: {foo: 1}, second: {foo: -1}}", "map[\"second\"].foo");
}

TEST(Traversal, TracksNumericMapOfObjectsDuringParsingAndValidation) {
    expect_error_at("numeric_map: {-11: {foo: nope}}", "numeric_map[-11].foo");
    expect_error_at("numeric_map: {-11: {foo: -1}}", "numeric_map[-11].foo");
}

TEST(Traversal, TracksNestedContainersDuringParsingAndValidation) {
    expect_error_at("nested: [{main: [{foo: 1}, {foo: nope}]}]", "nested[0][\"main\"][1].foo");
    expect_error_at("nested: [{main: [{foo: 1}, {foo: -1}]}]", "nested[0][\"main\"][1].foo");
}

TEST(Traversal, PreservesAliasesDuringParsingAndValidation) {
    expect_error_at("my.object: {f: nope}", "\"my.object\".f");
    expect_error_at("my.object: {f: -1}", "\"my.object\".f");
}

TEST(Traversal, TracksMissingFieldsAndFailedDefaults) {
    expect_error_at("array: [{}]", "array[0].foo", KEY_NOT_FOUND);
    expect_error_at("defaults: {}", "defaults.foo");
}

TEST(Traversal, TracksContainerAndChildLookupFailures) {
    expect_error_at("array: nope", "array", WRONG_TYPE);
    expect_error_at("map: nope", "map", WRONG_TYPE);
    expect_error_at("array: [42]", "array[0].foo", WRONG_TYPE);
    expect_error_at("numeric_map: {nope: {foo: 1}}", "numeric_map");
}

TEST(Traversal, DoesNotRetainEarlierFieldPathsOrRepeatContext) {
    const auto result = load<Tree>(YamlParser{YAML::Load(
        "object: {foo: 1}\narray: [{foo: 1}, {foo: -1}]")});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().what(), "array[1].foo: expected positive");
}

TEST(Traversal, MaterializesValidNestedValues) {
    const auto result = load<Tree>(YamlParser{YAML::Load(
        "object: {foo: 1}\narray: [{foo: 2}]\nmap: {main: {foo: 3}}\n"
        "numeric_map: {-11: {foo: 4}}\nnested: [{main: [{foo: 5}]}]")});
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().object.foo, 1);
    EXPECT_EQ(result.value().array.at(0).foo, 2);
    EXPECT_EQ(result.value().map.at("main").foo, 3);
    EXPECT_EQ(result.value().numeric_map.at(-11).foo, 4);
    EXPECT_EQ(result.value().nested.at(0).at("main").at(0).foo, 5);
}

} // namespace
