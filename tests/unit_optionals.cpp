#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "struo/struo.hpp"

namespace {

struct Endpoint {
    std::string host;
    int port{};
};

template<typename T>
struct Config {
    std::optional<T> value;
};

struct InitializedConfig {
    std::optional<int> number{42};
    std::optional<Endpoint> object{Endpoint{"original", 9000}};
    std::optional<std::vector<int>> sequence{std::vector<int>{1, 2}};
    std::optional<std::map<std::string, int>> map{std::map<std::string, int>{{"original", 3}}};
};

struct RequiredConfig {
    std::optional<int> value{42};
};

struct DefaultConfig {
    std::optional<int> value{42};
    std::optional<int> cleared{42};
};

struct ConstrainedConfig {
    std::optional<int> value;
};

using Choice = std::variant<int, std::optional<int>>;
enum class Kind { NUMBER, OPTIONAL_NUMBER };

constexpr auto PositiveNumber = [](const std::optional<int>& value) -> struo::Result<void> {
    if (value && *value <= 0) {
        return struo::err(struo::INVALID_VALUE, "number must be positive");
    }
    return struo::ok();
};

} // namespace

namespace struo {

template<>
struct SchemaTraits<Endpoint> {
    static auto schema() {
        return Object{
            Field<&Endpoint::host>{Keys{"host"}, REQUIRED},
            Field<&Endpoint::port>{Keys{"port"}, Defaults{Value<8080>}, Constraints{IsValidPort}}
        };
    }
};

template<typename T>
struct SchemaTraits<Config<T>> {
    static auto schema() {
        return Object{Field<&Config<T>::value>{Keys{"value"}}};
    }
};

template<>
struct SchemaTraits<InitializedConfig> {
    static auto schema() {
        return Object{
            Field<&InitializedConfig::number>{Keys{"number"}},
            Field<&InitializedConfig::object>{Keys{"object"}},
            Field<&InitializedConfig::sequence>{Keys{"sequence"}},
            Field<&InitializedConfig::map>{Keys{"map"}}
        };
    }
};

template<>
struct SchemaTraits<RequiredConfig> {
    static auto schema() {
        return Object{Field<&RequiredConfig::value>{Keys{"value"}, REQUIRED}};
    }
};

template<>
struct SchemaTraits<DefaultConfig> {
    static auto schema() {
        return Object{
            Field<&DefaultConfig::value>{Keys{"value"}, REQUIRED,
                Defaults{[] { return std::optional<int>{9}; }}},
            Field<&DefaultConfig::cleared>{Keys{"cleared"}, REQUIRED,
                Defaults{[] { return std::optional<int>{}; }}}
        };
    }
};

template<>
struct SchemaTraits<ConstrainedConfig> {
    static auto schema() {
        return Object{Field<&ConstrainedConfig::value>{Keys{"value"}, Constraints{PositiveNumber}}};
    }
};

template<>
struct SchemaTraits<Choice> {
    static auto schema() {
        return Variant{Bindings{
            Bind<Kind::OPTIONAL_NUMBER, std::optional<int>>{},
            Bind<Kind::NUMBER, int>{}
        }};
    }
};

} // namespace struo

namespace {

template<typename T = Config<int>>
auto load_config(std::string_view yaml) {
    return struo::load<T>(struo::YamlParser{YAML::Load(std::string{yaml})});
}

TEST(Optionals, MissingFieldPreservesDisengagedValue) {
    const auto result = load_config("{}");
    ASSERT_TRUE(result);
    EXPECT_FALSE(result.value().value);
}

TEST(Optionals, MissingFieldsPreserveUserDefaults) {
    const auto result = load_config<InitializedConfig>("{}");
    ASSERT_TRUE(result);
    const auto& config = result.value();
    EXPECT_EQ(config.number, 42);
    ASSERT_TRUE(config.object);
    EXPECT_EQ(config.object->host, "original");
    EXPECT_EQ(config.object->port, 9000);
    ASSERT_TRUE(config.sequence);
    EXPECT_EQ(*config.sequence, (std::vector<int>{1, 2}));
    ASSERT_TRUE(config.map);
    EXPECT_EQ(*config.map, (std::map<std::string, int>{{"original", 3}}));
}

TEST(Optionals, PresentFieldsReplaceUserDefaults) {
    const auto result = load_config<InitializedConfig>(R"(
number: 7
object: {host: updated}
sequence: [4, 5]
map: {updated: 6}
)");
    ASSERT_TRUE(result);
    const auto& config = result.value();
    EXPECT_EQ(config.number, 7);
    ASSERT_TRUE(config.object);
    EXPECT_EQ(config.object->host, "updated");
    EXPECT_EQ(config.object->port, 8080);
    ASSERT_TRUE(config.sequence);
    EXPECT_EQ(*config.sequence, (std::vector<int>{4, 5}));
    ASSERT_TRUE(config.map);
    EXPECT_EQ(*config.map, (std::map<std::string, int>{{"updated", 6}}));
}

TEST(Optionals, MaterializesInteger) {
    const auto result = load_config("value: 42");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().value, 42);
}

TEST(Optionals, MaterializesString) {
    const auto result = load_config<Config<std::string>>("value: hello");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().value, "hello");
}

TEST(Optionals, MaterializesFloatingPoint) {
    const auto result = load_config<Config<double>>("value: 1.25");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    EXPECT_DOUBLE_EQ(*result.value().value, 1.25);
}

TEST(Optionals, FalseAndZeroRemainEngaged) {
    const auto boolean = load_config<Config<bool>>("value: false");
    ASSERT_TRUE(boolean);
    ASSERT_TRUE(boolean.value().value.has_value());
    EXPECT_FALSE(*boolean.value().value);

    const auto number = load_config("value: 0");
    ASSERT_TRUE(number);
    EXPECT_EQ(number.value().value, 0);
}

TEST(Optionals, EmptyStringRemainsEngaged) {
    const auto result = load_config<Config<std::string>>("value: ''");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    EXPECT_TRUE(result.value().value->empty());
}

TEST(Optionals, MaterializesObject) {
    const auto result = load_config<Config<Endpoint>>("value: {host: db.local, port: 5432}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    EXPECT_EQ(result.value().value->host, "db.local");
    EXPECT_EQ(result.value().value->port, 5432);
}

TEST(Optionals, MaterializesSequence) {
    const auto result = load_config<Config<std::vector<int>>>("value: [1, 2, 3]");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    EXPECT_EQ(*result.value().value, (std::vector<int>{1, 2, 3}));
}

TEST(Optionals, MaterializesSequenceOfObjects) {
    const auto result = load_config<Config<std::vector<Endpoint>>>(
        "value: [{host: first}, {host: second, port: 9000}]");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    const auto& endpoints = *result.value().value;
    ASSERT_EQ(endpoints.size(), 2);
    EXPECT_EQ(endpoints[0].host, "first");
    EXPECT_EQ(endpoints[0].port, 8080);
    EXPECT_EQ(endpoints[1].host, "second");
    EXPECT_EQ(endpoints[1].port, 9000);
}

TEST(Optionals, MaterializesMapOfObjects) {
    const auto result = load_config<Config<std::map<std::string, Endpoint>>>(
        "value: {primary: {host: db.local}, backup: {host: backup.local, port: 9000}}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    const auto& endpoints = *result.value().value;
    ASSERT_EQ(endpoints.size(), 2);
    EXPECT_EQ(endpoints.at("primary").host, "db.local");
    EXPECT_EQ(endpoints.at("primary").port, 8080);
    EXPECT_EQ(endpoints.at("backup").host, "backup.local");
    EXPECT_EQ(endpoints.at("backup").port, 9000);
}

TEST(Optionals, EmptyContainersRemainEngaged) {
    const auto sequence = load_config<Config<std::vector<int>>>("value: []");
    ASSERT_TRUE(sequence);
    ASSERT_TRUE(sequence.value().value);
    EXPECT_TRUE(sequence.value().value->empty());

    const auto map = load_config<Config<std::map<std::string, int>>>("value: {}");
    ASSERT_TRUE(map);
    ASSERT_TRUE(map.value().value);
    EXPECT_TRUE(map.value().value->empty());
}

TEST(Optionals, MaterializesOptionalSequenceElements) {
    const auto result = load_config<Config<std::vector<std::optional<int>>>>("value: [0, 2, 3]");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    EXPECT_EQ(*result.value().value, (std::vector<std::optional<int>>{0, 2, 3}));
}

TEST(Optionals, MaterializesOptionalMapValues) {
    const auto result = load_config<Config<std::map<std::string, std::optional<Endpoint>>>>(
        "value: {primary: {host: db.local}}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    const auto& endpoints = *result.value().value;
    ASSERT_EQ(endpoints.size(), 1);
    ASSERT_TRUE(endpoints.at("primary"));
    EXPECT_EQ(endpoints.at("primary")->host, "db.local");
    EXPECT_EQ(endpoints.at("primary")->port, 8080);
}

TEST(Optionals, MaterializesNestedOptional) {
    const auto result = load_config<Config<std::optional<int>>>("value: 42");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    EXPECT_EQ(*result.value().value, 42);
}

TEST(Optionals, MaterializesVariant) {
    const auto result = load_config<Config<Choice>>("value: {type: NUMBER, value: 42}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    const auto& choice = *result.value().value;
    ASSERT_TRUE(std::holds_alternative<int>(choice));
    EXPECT_EQ(std::get<int>(choice), 42);
}

TEST(Optionals, MaterializesOptionalVariantAlternativeWithSharedStagedType) {
    const auto result = load_config<Config<Choice>>("value: {type: OPTIONAL_NUMBER, value: 42}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().value);
    const auto& choice = *result.value().value;
    ASSERT_TRUE(std::holds_alternative<std::optional<int>>(choice));
    EXPECT_EQ(std::get<std::optional<int>>(choice), 42);
}

TEST(Optionals, SchemaDefaultsCanSupplyOrClearValue) {
    const auto result = load_config<DefaultConfig>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().value, 9);
    EXPECT_FALSE(result.value().cleared);
}

TEST(Optionals, PresentValueOverridesSchemaDefault) {
    const auto result = load_config<DefaultConfig>("value: 7\ncleared: 8");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().value, 7);
    EXPECT_EQ(result.value().cleared, 8);
}

TEST(Optionals, MissingRequiredFieldFailsDespiteUserDefault) {
    const auto result = load_config<RequiredConfig>("{}");
    ASSERT_FALSE(result);
}

TEST(Optionals, PresentRequiredFieldSucceeds) {
    const auto result = load_config<RequiredConfig>("value: 7");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().value, 7);
}

TEST(Optionals, InvalidScalarFailsInsteadOfUsingDefault) {
    const auto result = load_config<DefaultConfig>("value: invalid");
    ASSERT_FALSE(result);
}

TEST(Optionals, InvalidSequenceShapeFails) {
    const auto result = load_config<Config<std::vector<int>>>("value: {wrong: shape}");
    ASSERT_FALSE(result);
}

TEST(Optionals, InvalidSequenceElementFails) {
    const auto result = load_config<Config<std::vector<std::optional<int>>>>("value: [1, invalid]");
    ASSERT_FALSE(result);
}

TEST(Optionals, InvalidMapShapeFails) {
    const auto result = load_config<Config<std::map<std::string, int>>>("value: [1, 2]");
    ASSERT_FALSE(result);
}

TEST(Optionals, MissingRequiredObjectFieldFails) {
    const auto result = load_config<Config<Endpoint>>("value: {}");
    ASSERT_FALSE(result);
}

TEST(Optionals, InnerObjectConstraintFailurePropagates) {
    const auto result = load_config<Config<Endpoint>>("value: {host: db.local, port: -1}");
    ASSERT_FALSE(result);
}

TEST(Optionals, FieldConstraintReceivesOptionalValue) {
    const auto valid = load_config<ConstrainedConfig>("value: 7");
    ASSERT_TRUE(valid);
    EXPECT_EQ(valid.value().value, 7);

    const auto invalid = load_config<ConstrainedConfig>("value: 0");
    ASSERT_FALSE(invalid);
}

TEST(Optionals, InvalidVariantContentFails) {
    const auto result = load_config<Config<Choice>>("value: {type: OPTIONAL_NUMBER, value: invalid}");
    ASSERT_FALSE(result);
}

} // namespace
