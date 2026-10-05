#include <gtest/gtest.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <map>

#include "struo/constraints/value.hpp"
#include "struo/struo.hpp"

TEST(ValueConstraints, Empty) {
    std::unordered_map<std::string, int> map{};
    std::vector<int> array{};
    std::string str{};

    EXPECT_FALSE(struo::NotEmpty(map));
    EXPECT_FALSE(struo::NotEmpty(array));
    EXPECT_FALSE(struo::NotEmpty(str));
}

TEST(ValueConstraints, NotEmpty) {
    std::unordered_map<std::string, int> map{ { "hello", 1 }};
    std::vector<int> array{ 1, 2, 3 };
    std::string str{"hello"};

    EXPECT_TRUE(struo::NotEmpty(map));
    EXPECT_TRUE(struo::NotEmpty(array));
    EXPECT_TRUE(struo::NotEmpty(str));
}

TEST(ValueConstraints, PositiveConstraintNegativeValues) {
    double value { 0.0 };
    int int_value { 0 };

    EXPECT_FALSE(struo::Positive(value));
    EXPECT_FALSE(struo::Positive(int_value));
}

TEST(ValueConstraints, PositiveConstraintPositiveValues) {
    double value { 100.0 };
    int int_value { 1 };

    EXPECT_TRUE(struo::Positive(value));
    EXPECT_TRUE(struo::Positive(int_value));
}

TEST(ValueConstraints, OneOfMatches) {
    auto result = struo::OneOf<1, 2, 3, 4>(1);
    EXPECT_TRUE(result) << result.error().what();

    result = struo::OneOf<1, 2, 3, 4>(5);
    EXPECT_FALSE(result);
}

TEST(ValueConstraints, StartsWithSuccess) {
    std::string str { "hello world" };
    auto result = struo::StartsWith<struo::Str{"hello"}>(str);
    EXPECT_TRUE(result) << result.error().what();
}

TEST(ValueConstraints, StartsWithFailure) {
    std::string str { "hello world" };
    auto result = struo::StartsWith<struo::Str{"1234"}>(str);
    EXPECT_FALSE(result) << result.error().what();
}

TEST(ValueConstraints, EndsWithSuccess) {
    std::string str { "hello world" };
    auto result = struo::EndsWith<struo::Str{"world"}>(str);
    EXPECT_TRUE(result) << result.error().what();
}

TEST(ValueConstraints, EndssWithFailure) {
    std::string str { "hello world" };
    auto result = struo::EndsWith<struo::Str{"1234"}>(str);
    EXPECT_FALSE(result) << result.error().what();
}
