#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "struo/struo.hpp"
#include "scoped.hpp"

namespace {

struct PresenceConfig {
    int required{42};
    int optional{24};
};

struct Database {
    std::string host{"user-defined-host"};
    int port{};
};

struct Config {
    std::string name;
    int workers{};
    Database primary;
    std::vector<Database> replicas;
    std::map<std::string, Database> databases;
};

struct InvalidDefaultConfig {
    int value{};
};

struct EnvironmentConfig {
    int workers{};
};

} // namespace

namespace struo {

template<>
struct SchemaTraits<Database> {
    static auto schema() {
        return Object{
            Field<&Database::host>{Keys{"host", "h"}, Constraints{NotEmpty}},
            Field<&Database::port>{Keys{"port", "p"}, Defaults{Value<1111>}, Constraints{IsValidPort}}
        };
    }
};

template<>
struct SchemaTraits<PresenceConfig> {
    static auto schema() {
        return Object{
            Field<&PresenceConfig::required>{Keys{"required"}, REQUIRED},
            Field<&PresenceConfig::optional>{Keys{"optional"}, OPTIONAL}
        };
    }
};

template<>
struct SchemaTraits<Config> {
    static auto schema() {
        return Object{
            Field<&Config::name>{Keys{"name"}, Defaults{[] { return "my database"; }}, Constraints{NotEmpty}},
            Field<&Config::workers>{Keys{"workers", "threads"}, Defaults{Value<12>}, Constraints{AtLeast<1>, AtMost<64>}},
            Field<&Config::primary>{Keys{"primary", "main"}},
            Field<&Config::replicas>{Keys{"replicas", "r"}, Constraints{SizeAtMost<2>}},
            Field<&Config::databases>{Keys{"databases", "dbs"}, Constraints{SizeAtMost<2>}}
        };
    }
};

template<>
struct SchemaTraits<InvalidDefaultConfig> {
    static auto schema() {
        return Object{
            Field<&InvalidDefaultConfig::value>{Keys{"value"}, Defaults{Value<5>}, Constraints{AtLeast<10>}}
        };
    }
};

#if STRUO_PLATFORM_LINUX
template<>
struct SchemaTraits<EnvironmentConfig> {
    static auto schema() {
        return Object{
            Field<&EnvironmentConfig::workers>{Keys{"workers"}, Defaults{FromEnv<"STRUO_TEST_ENV_WORKERS">}, Constraints{AtLeast<1>, AtMost<64>}}
        };
    }
};
#endif

} // namespace struo

namespace {

using namespace struo;

TEST(Parsing, PropagatesWrongSequenceType) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
replicas:
  first:
    host: replica-a
    port: 5000
)")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
}

TEST(Parsing, PropagatesInvalidScalarValue) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
workers: definitely-not-an-int
)")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), INVALID_VALUE);
}

TEST(Parsing, MaterializesCompleteObject) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
name: production
workers: 16

primary:
  host: primary.local
  port: 5432

replicas:
  - host: replica-a.local
    port: 5433
  - host: replica-b.local
    port: 5434

databases:
  analytics:
    host: analytics.local
    port: 6000
  archive:
    host: archive.local
    port: 6001
)")});

    ASSERT_TRUE(result);

    const auto& config = *result;

    EXPECT_EQ(config.name, "production");
    EXPECT_EQ(config.workers, 16);

    EXPECT_EQ(config.primary.host, "primary.local");
    EXPECT_EQ(config.primary.port, 5432);

    ASSERT_EQ(config.replicas.size(), 2);
    EXPECT_EQ(config.replicas[0].host, "replica-a.local");
    EXPECT_EQ(config.replicas[0].port, 5433);
    EXPECT_EQ(config.replicas[1].host, "replica-b.local");
    EXPECT_EQ(config.replicas[1].port, 5434);

    ASSERT_EQ(config.databases.size(), 2);
    EXPECT_EQ(config.databases.at("analytics").host, "analytics.local");
    EXPECT_EQ(config.databases.at("analytics").port, 6000);
    EXPECT_EQ(config.databases.at("archive").host, "archive.local");
    EXPECT_EQ(config.databases.at("archive").port, 6001);
}

TEST(Parsing, LoadsFieldsUsingAlternateKeys) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
name: production
threads: 8

main:
  h: primary.local
  p: 5432

r:
  - h: replica-a.local
    p: 5433

dbs:
  analytics:
    h: analytics.local
    p: 6000
)")});

    ASSERT_TRUE(result);

    const auto& config = *result;

    EXPECT_EQ(config.name, "production");
    EXPECT_EQ(config.workers, 8);

    EXPECT_EQ(config.primary.host, "primary.local");
    EXPECT_EQ(config.primary.port, 5432);

    ASSERT_EQ(config.replicas.size(), 1);
    EXPECT_EQ(config.replicas[0].host, "replica-a.local");
    EXPECT_EQ(config.replicas[0].port, 5433);

    ASSERT_EQ(config.databases.size(), 1);
    EXPECT_EQ(config.databases.at("analytics").host, "analytics.local");
    EXPECT_EQ(config.databases.at("analytics").port, 6000);
}

TEST(Parsing, MaterializesDefaultsForMissingFields) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
primary: {}
)")});

    ASSERT_TRUE(result);

    const auto& config = *result;

    EXPECT_EQ(config.name, "my database");
    EXPECT_EQ(config.workers, 12);
    EXPECT_EQ(config.primary.host, "user-defined-host");
    EXPECT_EQ(config.primary.port, 1111);
    EXPECT_TRUE(config.replicas.empty());
    EXPECT_TRUE(config.databases.empty());
}

TEST(Parsing, MaterializesDefaultsInSequenceAndMapElements) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
replicas:
  - {}
  - host: replica.local

databases:
  primary: {}
  backup:
    port: 6000
)")});

    ASSERT_TRUE(result);

    const auto& config = *result;

    ASSERT_EQ(config.replicas.size(), 2);
    EXPECT_EQ(config.replicas[0].host, "user-defined-host");
    EXPECT_EQ(config.replicas[0].port, 1111);
    EXPECT_EQ(config.replicas[1].host, "replica.local");
    EXPECT_EQ(config.replicas[1].port, 1111);

    ASSERT_EQ(config.databases.size(), 2);
    EXPECT_EQ(config.databases.at("primary").host, "user-defined-host");
    EXPECT_EQ(config.databases.at("primary").port, 1111);
    EXPECT_EQ(config.databases.at("backup").host, "user-defined-host");
    EXPECT_EQ(config.databases.at("backup").port, 6000);
}

TEST(Parsing, RejectsScalarValueFailingConstraint) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
workers: 65
)")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ARGUMENT_OUT_OF_RANGE);
}

TEST(Parsing, RejectsEmptyStringFailingConstraint) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
name: ""
)")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ARGUMENT_OUT_OF_RANGE);
}

TEST(Parsing, RejectsNestedValueFailingConstraint) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
primary:
  port: 70000
)")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ARGUMENT_OUT_OF_RANGE);
}

TEST(Parsing, RejectsContainerFailingConstraint) {
    auto result = load<Config>(YamlParser{YAML::Load(R"(
replicas:
  - {}
  - {}
  - {}
)")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ARGUMENT_OUT_OF_RANGE);
}

TEST(Parsing, AppliesConstraintsToDefaultValues) {
    auto result = load<InvalidDefaultConfig>(YamlParser{YAML::Load("{}")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ARGUMENT_OUT_OF_RANGE);
}

TEST(Parsing, MissingRequiredFieldFails) {
    auto result = load<PresenceConfig>(YamlParser{YAML::Load(R"(
optional: 10
)")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), KEY_NOT_FOUND);
}

TEST(Parsing, MissingOptionalFieldPreservesUserValue) {
    auto result = load<PresenceConfig>(YamlParser{YAML::Load(R"(
required: 10
)")});

    ASSERT_TRUE(result);

    EXPECT_EQ(result.value().required, 10);
    EXPECT_EQ(result.value().optional, 24);
}

TEST(Parsing, MaterializesRequiredAndOptionalFieldsWhenPresent) {
    auto result = load<PresenceConfig>(YamlParser{YAML::Load(R"(
required: 10
optional: 20
)")});

    ASSERT_TRUE(result);

    EXPECT_EQ(result.value().required, 10);
    EXPECT_EQ(result.value().optional, 20);
}

#if STRUO_PLATFORM_LINUX

TEST(Parsing, MaterializesEnvironmentVariableDefault) {
    testlib::ScopedEnvVariable env{"STRUO_TEST_ENV_WORKERS", "24"};

    auto result = load<EnvironmentConfig>(YamlParser{YAML::Load("{}")});

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().workers, 24);
}

TEST(Parsing, AppliesConstraintsToEnvironmentVariableDefaults) {
    testlib::ScopedEnvVariable env{"STRUO_TEST_ENV_WORKERS", "100"};

    auto result = load<EnvironmentConfig>(YamlParser{YAML::Load("{}")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ARGUMENT_OUT_OF_RANGE);
}

TEST(Parsing, PropagatesInvalidEnvironmentVariableConversion) {
    testlib::ScopedEnvVariable env{"STRUO_TEST_ENV_WORKERS", "not-an-integer"};

    auto result = load<EnvironmentConfig>(YamlParser{YAML::Load("{}")});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
}

TEST(Parsing, ExplicitValueOverridesEnvironmentVariableDefault) {
    testlib::ScopedEnvVariable env{"STRUO_TEST_ENV_WORKERS", "24"};

    auto result = load<EnvironmentConfig>(YamlParser{YAML::Load(R"(
workers: 32
)")});

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().workers, 32);
}

#endif

} // namespace
