#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "struo/Field.hpp"
#include "struo/Object.hpp"
#include "struo/forward.hpp"
#include "struo/parsing/YamlParser.hpp"
#include "struo/parsing/parsing.hpp"

namespace {

struct Database {
    // Ordinary user-defined C++ fallback.
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

} // namespace

namespace struo {

template<>
struct SchemaTraits<Database> {
    static auto schema() {
        return Object{
            Field<&Database::host>{
                Keys{"host", "h"}
            },
            Field<&Database::port>{
                Keys{"port", "p"},
                Defaults{
                    Value<1111>
                }
            }
        };
    }
};

template<>
struct SchemaTraits<Config> {
    static auto schema() {
        return Object{
            Field<&Config::name>{
                Keys{"name"},
                Defaults{
                    [] { return "my database"; }
                }
            },
            Field<&Config::workers>{
                Keys{"workers", "threads"},
                Defaults{
                    Value<12>
                }
            },
            Field<&Config::primary>{
                Keys{"primary", "main"}
            },
            Field<&Config::replicas>{
                Keys{"replicas", "r"}
            },
            Field<&Config::databases>{
                Keys{"databases", "dbs"}
            }
        };
    }
};

} // namespace struo

namespace {

using namespace struo;

TEST(Parsing, PropagatesWrongSequenceType) {
    auto result = load<Config>(
        YamlParser{YAML::Load(R"(
replicas:
  first:
    host: replica-a
    port: 5000
)")}
    );

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
}

TEST(Parsing, PropagatesInvalidScalarValue) {
    auto result = load<Config>(
        YamlParser{YAML::Load(R"(
workers: definitely-not-an-int
)")}
    );

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), INVALID_VALUE);
}

TEST(Parsing, MaterializesCompleteObject) {
    auto result = load<Config>(
        YamlParser{YAML::Load(R"(
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
)")}
    );

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

    EXPECT_EQ(
        config.databases.at("analytics").host,
        "analytics.local"
    );
    EXPECT_EQ(
        config.databases.at("analytics").port,
        6000
    );

    EXPECT_EQ(
        config.databases.at("archive").host,
        "archive.local"
    );
    EXPECT_EQ(
        config.databases.at("archive").port,
        6001
    );
}

TEST(Parsing, LoadsFieldsUsingAlternateKeys) {
    auto result = load<Config>(
        YamlParser{YAML::Load(R"(
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
)")}
    );

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
    EXPECT_EQ(
        config.databases.at("analytics").host,
        "analytics.local"
    );
    EXPECT_EQ(
        config.databases.at("analytics").port,
        6000
    );
}

TEST(Parsing, MaterializesDefaultsForMissingFields) {
    auto result = load<Config>(
        YamlParser{YAML::Load(R"(
primary: {}
)")}
    );

    ASSERT_TRUE(result);

    const auto& config = *result;

    // Callable default.
    EXPECT_EQ(config.name, "my database");

    // Value<V> default.
    EXPECT_EQ(config.workers, 12);

    // No Struo default: preserve the value produced by Database{}.
    EXPECT_EQ(config.primary.host, "user-defined-host");

    // Nested Struo default.
    EXPECT_EQ(config.primary.port, 1111);

    EXPECT_TRUE(config.replicas.empty());
    EXPECT_TRUE(config.databases.empty());
}

TEST(Parsing, MaterializesDefaultsInSequenceAndMapElements) {
    auto result = load<Config>(
        YamlParser{YAML::Load(R"(
replicas:
  - {}
  - host: replica.local

databases:
  primary: {}
  backup:
    port: 6000
)")}
    );

    ASSERT_TRUE(result);

    const auto& config = *result;

    ASSERT_EQ(config.replicas.size(), 2);

    // Both values absent.
    EXPECT_EQ(
        config.replicas[0].host,
        "user-defined-host"
    );
    EXPECT_EQ(
        config.replicas[0].port,
        1111
    );

    // Explicit host, default port.
    EXPECT_EQ(
        config.replicas[1].host,
        "replica.local"
    );
    EXPECT_EQ(
        config.replicas[1].port,
        1111
    );

    ASSERT_EQ(config.databases.size(), 2);

    // Both values absent.
    EXPECT_EQ(
        config.databases.at("primary").host,
        "user-defined-host"
    );
    EXPECT_EQ(
        config.databases.at("primary").port,
        1111
    );

    // User fallback host, explicit port.
    EXPECT_EQ(
        config.databases.at("backup").host,
        "user-defined-host"
    );
    EXPECT_EQ(
        config.databases.at("backup").port,
        6000
    );
}

} // namespace
