#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "struo/struo.hpp"

namespace {

struct YamlScalarInput {
    static auto parse(std::string_view text) {
        return struo::Yaml{YAML::Load(std::string{text})};
    }
    static constexpr bool supports_null = true;
    static auto document(std::string_view text, std::string_view) { return parse(text); }
};

struct JsonScalarInput {
    static auto parse(std::string_view text) {
        return struo::Json{nlohmann::json::parse(text)};
    }
    static constexpr bool supports_null = true;
    static auto document(std::string_view text, std::string_view) { return parse(text); }
};

struct TomlScalarInput {
    toml::table source;

    auto parse(std::string_view text) {
        source = toml::parse("value = " + std::string{text});
        return struo::Toml{*source.get("value")};
    }
    static constexpr bool supports_null = false;
    auto document(std::string_view, std::string_view text) {
        source = toml::parse(text);
        return struo::Toml{source};
    }
};

struct ScalarTypesConfig {
    std::chrono::milliseconds delay{};
    std::chrono::seconds timeout{};
    std::chrono::duration<double> interval{};
    std::optional<std::filesystem::path> output;
    std::vector<std::filesystem::path> inputs;
    std::map<std::string, std::chrono::milliseconds> timers;
};

static_assert(struo::IsChronoDuration<const std::chrono::milliseconds&>);
static_assert(!struo::IsChronoDuration<int>);
static_assert(struo::IsScalar<std::chrono::seconds>);
static_assert(struo::IsScalar<std::filesystem::path>);
static_assert(!struo::IsScalar<std::vector<int>>);

} // namespace

namespace struo {

template<>
struct SchemaTraits<ScalarTypesConfig> {
    static auto schema() {
        return Object{
            Field<&ScalarTypesConfig::delay>{Keys{"delay"}},
            Field<&ScalarTypesConfig::timeout>{Keys{"timeout"}, Defaults{[] { return std::chrono::seconds{5}; }}},
            Field<&ScalarTypesConfig::interval>{Keys{"interval"}},
            Field<&ScalarTypesConfig::output>{Keys{"output"}},
            Field<&ScalarTypesConfig::inputs>{Keys{"inputs"}},
            Field<&ScalarTypesConfig::timers>{Keys{"timers"}}
        };
    }
};

} // namespace struo

namespace {

template<typename Input>
class ScalarTypes : public testing::Test {};

using ScalarInputs = testing::Types<YamlScalarInput, JsonScalarInput, TomlScalarInput>;
TYPED_TEST_SUITE(ScalarTypes, ScalarInputs);

TYPED_TEST(ScalarTypes, ParsesCountsInTheRequestedDurationUnits) {
    TypeParam input;
    const auto parser = input.parse("250");
    const auto milliseconds = parser.template getAs<std::chrono::milliseconds>();
    const auto seconds = parser.template getAs<std::chrono::seconds>();
    ASSERT_TRUE(milliseconds);
    ASSERT_TRUE(seconds);
    EXPECT_EQ(milliseconds.value(), std::chrono::milliseconds{250});
    EXPECT_EQ(seconds.value(), std::chrono::seconds{250});

    const auto fractional = input.parse("1.25").template getAs<std::chrono::duration<double>>();
    ASSERT_TRUE(fractional);
    EXPECT_DOUBLE_EQ(fractional.value().count(), 1.25);
    const auto negative = input.parse("-2").template getAs<std::chrono::seconds>();
    ASSERT_TRUE(negative);
    EXPECT_EQ(negative.value().count(), -2);
}

TYPED_TEST(ScalarTypes, ParsesUtf8PathsWithoutFilesystemAccess) {
    TypeParam input;
    const auto result = input.parse(R"("assets/caf\u00e9/data.txt")").template getAs<std::filesystem::path>();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), std::filesystem::path{u8"assets/café/data.txt"});
    const auto empty = input.parse(R"("")").template getAs<std::filesystem::path>();
    ASSERT_TRUE(empty);
    EXPECT_TRUE(empty.value().empty());
}

TYPED_TEST(ScalarTypes, PropagatesConversionAndShapeErrors) {
    TypeParam input;
    const auto invalid = input.parse(R"("slow")").template getAs<std::chrono::seconds>();
    ASSERT_FALSE(invalid);
    EXPECT_EQ(invalid.error().code(), struo::INVALID_VALUE);
    for(const auto text : {"[]", "{}", "null"}) {
        if(!TypeParam::supports_null && std::string_view{text} == "null") {
            continue;
        }
        const auto parser = input.parse(text);
        const auto duration = parser.template getAs<std::chrono::seconds>();
        ASSERT_FALSE(duration);
        EXPECT_EQ(duration.error().code(), struo::WRONG_TYPE);
        const auto path = parser.template getAs<std::filesystem::path>();
        ASSERT_FALSE(path);
        EXPECT_EQ(path.error().code(), struo::WRONG_TYPE);
    }
}

TYPED_TEST(ScalarTypes, LoadsDurationsAndPathsWithDefaultsAndContainers) {
    TypeParam input;
    const auto result = struo::load<ScalarTypesConfig>(input.document(R"({
        "delay": 250,
        "interval": 1.25,
        "output": "out/data.txt",
        "inputs": ["first.txt", "second.txt"],
        "timers": {"retry": 100}
    })", R"(
        delay = 250
        interval = 1.25
        output = "out/data.txt"
        inputs = ["first.txt", "second.txt"]
        [timers]
        retry = 100
    )"));
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().delay, std::chrono::milliseconds{250});
    EXPECT_EQ(result.value().timeout, std::chrono::seconds{5});
    EXPECT_DOUBLE_EQ(result.value().interval.count(), 1.25);
    ASSERT_TRUE(result.value().output);
    EXPECT_EQ(*result.value().output, std::filesystem::path{"out/data.txt"});
    EXPECT_EQ(result.value().inputs, (std::vector<std::filesystem::path>{"first.txt", "second.txt"}));
    EXPECT_EQ(result.value().timers.at("retry"), std::chrono::milliseconds{100});
}

TYPED_TEST(ScalarTypes, ReportsThePathOfInvalidDurationCounts) {
    TypeParam input;
    const auto result = struo::load<ScalarTypesConfig>(input.document(
        R"({"timers": {"retry": "slow"}})", R"(timers = {retry = "slow"})"));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), struo::INVALID_VALUE);
    EXPECT_TRUE(result.error().what().starts_with("timers[\"retry\"]: ")) << result.error().what();
}

} // namespace
