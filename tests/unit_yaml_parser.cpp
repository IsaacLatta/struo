#include <gtest/gtest.h>

#include "struo/parsing/parsing.hpp"

TEST(UnitYamlParser, ParsesScalars) {
    auto node = YAML::Load(R"(
first_name: jason
last_name: bourne
job: special_agent
)");

    struo::YamlParser parser { node };
    std::array keys { "first_name", "last_name", "job" };
    std::array values { "jason", "bourne", "special_agent" };
    for (size_t i { 0 }; i < keys.size(); ++i) {
        auto child_node = parser.toChild(keys.at(i));
        ASSERT_TRUE(child_node);
        ASSERT_TRUE(*child_node);

        auto parsed = child_node.value().value().getAs<std::string>();
        ASSERT_TRUE(parsed);
        ASSERT_TRUE(parsed);
        ASSERT_EQ(parsed.value(), values.at(i));
    }
}