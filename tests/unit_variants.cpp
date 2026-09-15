#include <gtest/gtest.h>

#include <list>
#include <map>
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

using Endpoints = std::vector<Endpoint>;
using NamedEndpoints = std::map<std::string, Endpoint>;
using Numbers = std::vector<int>;
using NumberList = std::list<int>;
using ConfigValue = std::variant<int, std::string, Endpoint, Endpoints,
                           NamedEndpoints, Numbers, NumberList>;
enum class Kind { NUMBER, TEXT, OBJECT, SEQUENCE, MAP, NUMBERS, LIST };

struct Config {
    ConfigValue choice{std::string{"original"}};
    std::vector<ConfigValue> choices;
    std::map<std::string, ConfigValue> named;
};

struct RequiredConfig {
    ConfigValue choice;
};

struct DefaultConfig {
    ConfigValue choice;
};

inline constexpr char nested_tag[] = "nested";
inline constexpr char enabled_tag[] = "enabled";
using NestedValue = std::variant<ConfigValue, bool>;

struct NestedConfig {
    NestedValue choice;
};

constexpr auto PositiveNumber = [](const ConfigValue& value) -> struo::Result<void> {
    if (const auto* number = std::get_if<int>(&value); number && *number <= 0) {
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
            Field<&Endpoint::host>{Keys{"host", "h"}, REQUIRED},
            Field<&Endpoint::port>{Keys{"port"}, Defaults{Value<8080>}, Constraints{IsValidPort}}
        };
    }
};

template<>
struct SchemaTraits<ConfigValue> {
    static auto schema() {
        // Deliberately differ from the destination variant's order.
        return Variant{Bindings{
            Bind<Kind::LIST, NumberList>{},
            Bind<Kind::MAP, NamedEndpoints>{},
            Bind<Kind::TEXT, std::string>{},
            Bind<Kind::NUMBERS, Numbers>{},
            Bind<Kind::SEQUENCE, Endpoints>{},
            Bind<Kind::OBJECT, Endpoint>{},
            Bind<Kind::NUMBER, int>{}
        }};
    }
};

template<>
struct SchemaTraits<Config> {
    static auto schema() {
        return Object{
            Field<&Config::choice>{Keys{"choice", "pick"}, Constraints{PositiveNumber}},
            Field<&Config::choices>{Keys{"choices"}},
            Field<&Config::named>{Keys{"named"}}
        };
    }
};

template<>
struct SchemaTraits<RequiredConfig> {
    static auto schema() {
        return Object{Field<&RequiredConfig::choice>{Keys{"choice"}, REQUIRED}};
    }
};

template<>
struct SchemaTraits<DefaultConfig> {
    static auto schema() {
        return Object{Field<&DefaultConfig::choice>{
            Keys{"choice"}, REQUIRED,
            Defaults{[] { return ConfigValue{42}; }},
            Constraints{PositiveNumber}
        }};
    }
};

template<>
struct SchemaTraits<NestedValue> {
    static auto schema() {
        return Variant{
            Bindings{
                Bind<enabled_tag, bool>{},
                Bind<nested_tag, ConfigValue>{}
            },
            TagKey{"kind"}, ContentKey{"config"}
        };
    }
};

template<>
struct SchemaTraits<NestedConfig> {
    static auto schema() {
        return Object{Field<&NestedConfig::choice>{Keys{"choice"}, REQUIRED}};
    }
};

} // namespace struo

namespace {

template<typename T = Config>
auto load_config(std::string_view yaml) {
    return struo::load<T>(struo::YamlParser{YAML::Load(std::string{yaml})});
}

template<typename T = Config>
void expect_error(std::string_view yaml) {
    const auto result = load_config<T>(yaml);
    ASSERT_FALSE(result);
}

TEST(Variants, MaterializesNumericScalar) {
    const auto result = load_config("choice: {type: NUMBER, value: 42}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<int>(result.value().choice));
    EXPECT_EQ(std::get<int>(result.value().choice), 42);
}

TEST(Variants, MaterializesStringScalar) {
    const auto result = load_config("choice: {type: TEXT, value: hello}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<std::string>(result.value().choice));
    EXPECT_EQ(std::get<std::string>(result.value().choice), "hello");
}

TEST(Variants, MaterializesObject) {
    const auto result = load_config("choice: {type: OBJECT, value: {host: db.local, port: 5432}}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<Endpoint>(result.value().choice));
    const auto& endpoint = std::get<Endpoint>(result.value().choice);
    EXPECT_EQ(endpoint.host, "db.local");
    EXPECT_EQ(endpoint.port, 5432);
}

TEST(Variants, MaterializesSequenceOfObjects) {
    const auto result = load_config(
        "choice: {type: SEQUENCE, value: [{host: first}, {host: second, port: 9000}]}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<Endpoints>(result.value().choice));
    const auto& endpoints = std::get<Endpoints>(result.value().choice);
    ASSERT_EQ(endpoints.size(), 2);
    EXPECT_EQ(endpoints[0].host, "first");
    EXPECT_EQ(endpoints[0].port, 8080);
    EXPECT_EQ(endpoints[1].host, "second");
    EXPECT_EQ(endpoints[1].port, 9000);
}

TEST(Variants, MaterializesMapOfObjects) {
    const auto result = load_config(
        "choice: {type: MAP, value: {primary: {host: db.local}, backup: {host: backup.local}}}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<NamedEndpoints>(result.value().choice));
    const auto& endpoints = std::get<NamedEndpoints>(result.value().choice);
    ASSERT_EQ(endpoints.size(), 2);
    EXPECT_EQ(endpoints.at("primary").host, "db.local");
    EXPECT_EQ(endpoints.at("primary").port, 8080);
    EXPECT_EQ(endpoints.at("backup").host, "backup.local");
}

TEST(Variants, DistinguishesAlternativesWithIdenticalStagedTypes) {
    const auto vector = load_config("choice: {type: NUMBERS, value: [1, 2, 3]}");
    const auto list = load_config("choice: {type: LIST, value: [4, 5, 6]}");
    ASSERT_TRUE(vector);
    ASSERT_TRUE(list);
    ASSERT_TRUE(std::holds_alternative<Numbers>(vector.value().choice));
    ASSERT_TRUE(std::holds_alternative<NumberList>(list.value().choice));
    EXPECT_EQ(std::get<Numbers>(vector.value().choice), (Numbers{1, 2, 3}));
    EXPECT_EQ(std::get<NumberList>(list.value().choice), (NumberList{4, 5, 6}));
}

TEST(Variants, AcceptsEmptyContainers) {
    const auto sequence = load_config("choice: {type: SEQUENCE, value: []}");
    const auto map = load_config("choice: {type: MAP, value: {}}");
    ASSERT_TRUE(sequence);
    ASSERT_TRUE(map);
    ASSERT_TRUE(std::holds_alternative<Endpoints>(sequence.value().choice));
    ASSERT_TRUE(std::holds_alternative<NamedEndpoints>(map.value().choice));
    EXPECT_TRUE(std::get<Endpoints>(sequence.value().choice).empty());
    EXPECT_TRUE(std::get<NamedEndpoints>(map.value().choice).empty());
}

TEST(Variants, MaterializesVariantsInsideContainers) {
    const auto result = load_config(R"(
choices:
  - {type: NUMBER, value: 12}
  - {type: TEXT, value: hello}
named:
  server: {type: OBJECT, value: {host: db.local}}
)");
    ASSERT_TRUE(result);
    ASSERT_EQ(result.value().choices.size(), 2);
    ASSERT_EQ(result.value().named.size(), 1);
    EXPECT_EQ(std::get<int>(result.value().choices[0]), 12);
    EXPECT_EQ(std::get<std::string>(result.value().choices[1]), "hello");
    EXPECT_EQ(std::get<Endpoint>(result.value().named.at("server")).host, "db.local");
}

TEST(Variants, SupportsStringTagsAndCustomKeys) {
    const auto result = load_config<NestedConfig>("choice: {kind: enabled, config: true}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<bool>(result.value().choice));
    EXPECT_TRUE(std::get<bool>(result.value().choice));
}

TEST(Variants, MaterializesNestedVariantAlternative) {
    const auto result = load_config<NestedConfig>(
        "choice: {kind: nested, config: {type: TEXT, value: hello}}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<ConfigValue>(result.value().choice));
    const auto& inner = std::get<ConfigValue>(result.value().choice);
    ASSERT_TRUE(std::holds_alternative<std::string>(inner));
    EXPECT_EQ(std::get<std::string>(inner), "hello");
}

TEST(Variants, AppliesObjectDefaultsAndAliases) {
    const auto result = load_config("pick: {type: OBJECT, value: {h: db.local}}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<Endpoint>(result.value().choice));
    EXPECT_EQ(std::get<Endpoint>(result.value().choice).host, "db.local");
    EXPECT_EQ(std::get<Endpoint>(result.value().choice).port, 8080);
}

TEST(Variants, PreservesMissingOptionalField) {
    const auto result = load_config("{}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<std::string>(result.value().choice));
    EXPECT_EQ(std::get<std::string>(result.value().choice), "original");
}

TEST(Variants, AppliesWholeVariantDefault) {
    const auto result = load_config<DefaultConfig>("{}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<int>(result.value().choice));
    EXPECT_EQ(std::get<int>(result.value().choice), 42);
}

TEST(Variants, ExplicitAlternativeOverridesDefault) {
    const auto result = load_config<DefaultConfig>("choice: {type: TEXT, value: explicit}");
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::holds_alternative<std::string>(result.value().choice));
    EXPECT_EQ(std::get<std::string>(result.value().choice), "explicit");
}

TEST(Variants, RejectsUnknownTagWithoutUsingDefault) {
    expect_error<DefaultConfig>("choice: {type: unknown, value: 12}");
}

TEST(Variants, RejectsNonScalarTag) {
    expect_error("choice: {type: [], value: 12}");
}

TEST(Variants, RejectsNonObjectEnvelope) {
    expect_error("choice: 12");
}

TEST(Variants, RejectsMissingContent) {
    expect_error("choice: {type: TEXT}");
}

TEST(Variants, RejectsInvalidScalarPayloadWithoutUsingDefault) {
    expect_error<DefaultConfig>("choice: {type: NUMBER, value: nope}");
}

TEST(Variants, RejectsWrongContainerPayloadShapes) {
    expect_error("choice: {type: SEQUENCE, value: {}}");
    expect_error("choice: {type: MAP, value: []}");
}

TEST(Variants, RejectsMissingFieldsWithinSelectedObject) {
    expect_error("choice: {type: OBJECT, value: {}}");
}

TEST(Variants, RejectsInvalidFieldsWithinSelectedObject) {
    expect_error("pick: {type: OBJECT, value: {h: [], port: 80}}");
    expect_error("pick: {type: OBJECT, value: {host: db.local, port: 70000}}");
}

TEST(Variants, AppliesWholeVariantConstraints) {
    const auto result = load_config("choice: {type: NUMBER, value: 0}");
    ASSERT_FALSE(result);
}

TEST(Variants, RejectsInvalidElementsInsideSequenceAndMapAlternatives) {
    expect_error("choice: {type: SEQUENCE, value: [{host: first}, {host: second, port: 70000}]}");
    expect_error("choice: {type: MAP, value: {backup: {host: []}}}");
}

TEST(Variants, RejectsInvalidVariantsInsideContainers) {
    expect_error("choices: [{type: TEXT, value: ok}, {type: unknown, value: 12}]");
    expect_error("named: {server: {type: OBJECT, value: {host: db.local, port: 70000}}}");
}

TEST(Variants, RejectsInvalidVariantsWithCustomKeysAndNesting) {
    expect_error<NestedConfig>("choice: {config: true}");
    expect_error<NestedConfig>("choice: {kind: enabled}");
    expect_error<NestedConfig>("choice: {kind: enabled, config: []}");
    expect_error<NestedConfig>("choice: {kind: nested, config: {type: OBJECT, value: {host: []}}}");
    expect_error<NestedConfig>("choice: {kind: nested, config: {type: OBJECT, value: {host: db.local, port: 70000}}}");
}

TEST(Variants, FormatsVariantErrorPath) {
    const auto result = load_config(R"(
choice: {type: OBJECT, value: {host: db.local}}
choices:
  - {type: TEXT, value: ok}
  - {type: OBJECT, value: {host: second, port: 70000}}
)");
    ASSERT_FALSE(result);
    EXPECT_TRUE(result.error().what().starts_with("choices[1]<OBJECT>.value.port: "))
        << result.error().what();
}

TEST(Variants, ChecksKeyConflictAfterApplyingAllOptions) {
    // Setting the tag first temporarily collides with the default content key.
    const auto schema = struo::Variant{
        struo::Bindings{struo::Bind<Kind::NUMBER, int>{}},
        struo::TagKey{"value"}, struo::ContentKey{"config"}
    };
    EXPECT_EQ(schema.getTagKey(), "value");
    EXPECT_EQ(schema.getContentKey(), "config");
}

#if !defined(STRUO_NO_ASSERT)
TEST(Variants, AssertsOnIdenticalTagAndContentKeys) {
    EXPECT_DEATH(([] {
        const auto schema = struo::Variant{
            struo::Bindings{struo::Bind<Kind::NUMBER, int>{}},
            struo::TagKey{"same"}, struo::ContentKey{"same"}
        };
        (void)schema;
    }()), ".*");
}
#endif

} // namespace
