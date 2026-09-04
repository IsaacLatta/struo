#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "struo/Field.hpp"
#include "struo/Object.hpp"
#include "struo/parsing/YamlParser.hpp"
#include "struo/parsing/parsing.hpp"

namespace {

struct Database {
    std::string host;
    int port;
};

struct Config {
    std::string name;
    int workers;

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
                Key{"host"}
            },
            Field<&Database::port>{
                Key{"port"}
            }
        };
    }
};

template<>
struct SchemaTraits<Config> {
    static auto schema() {
        return Object{
            Field<&Config::name>{
                Key{"name"}
            },
            Field<&Config::workers>{
                Key{"workers"}
            },
            Field<&Config::primary>{
                Key{"primary"}
            },
            Field<&Config::replicas>{
                Key{"replicas"}
            },
            Field<&Config::databases>{
                Key{"databases"}
            }
        };
    }
};

} // namespace struo

namespace {

using namespace struo;

TEST(ParseTraversal, PropagatesWrongSequenceType) {
    YamlParser parser{
        YAML::Load(R"(
replicas:
  first:
    host: replica-a
    port: 5000
)")
    };

    auto result = parse<Config>(parser);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), WRONG_TYPE);
}

TEST(ParseTraversal, PropagatesInvalidScalarValue) {
    YamlParser parser {YAML::Load(R"(
workers: definitely-not-an-int
)")
    };

    auto result = parse<Config>(parser);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), INVALID_VALUE);
}

TEST(Parsing, Instatiate) {
    auto result = struo::load<Config>(YamlParser {YAML::Load(R"(
workers: definitely-not-an-int
)")
    });

    ASSERT_FALSE(result);
}

} // namespace
