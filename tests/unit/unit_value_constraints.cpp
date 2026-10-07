#include "struo/constraints/value.hpp"

#include <gtest/gtest.h>

#include <array>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace {
using namespace struo;

void expectError(const Result<void>& result, ErrorCode code, std::string_view message) {
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), code);
    EXPECT_NE(result.error().what().find(message), std::string_view::npos) << result.error().what();
}

template<typename T>
class NumericValueConstraints : public ::testing::Test {};
using NumericTypes = ::testing::Types<int, long long, unsigned int, float, double, long double>;
TYPED_TEST_SUITE(NumericValueConstraints, NumericTypes);

TYPED_TEST(NumericValueConstraints, PositiveAcceptsPositiveAndRejectsZeroAndNegative) {
    using T = TypeParam;
    EXPECT_TRUE(Positive(T{1}));
    EXPECT_TRUE(Positive(std::numeric_limits<T>::max()));
    expectError(Positive(T{0}), ARGUMENT_OUT_OF_RANGE, "\"positive\"");
    if constexpr (std::is_signed_v<T>) {
        expectError(Positive(T{-1}), ARGUMENT_OUT_OF_RANGE, "\"positive\"");
    }
}

TYPED_TEST(NumericValueConstraints, IsFiniteAcceptsFiniteValues) {
    using T = TypeParam;
    EXPECT_TRUE(IsFinite(T{0}));
    EXPECT_TRUE(IsFinite(T{1}));
    EXPECT_TRUE(IsFinite(std::numeric_limits<T>::lowest()));
    EXPECT_TRUE(IsFinite(std::numeric_limits<T>::max()));
    if constexpr (std::is_floating_point_v<T>) {
        EXPECT_TRUE(IsFinite(T{-0.0}));
        EXPECT_TRUE(IsFinite(std::numeric_limits<T>::denorm_min()));
    }
}

template<typename T>
class FloatingValueConstraints : public ::testing::Test {};
using FloatingTypes = ::testing::Types<float, double, long double>;
TYPED_TEST_SUITE(FloatingValueConstraints, FloatingTypes);

TYPED_TEST(FloatingValueConstraints, FloatingPointSpecialValues) {
    using T = TypeParam;
    if constexpr (std::is_floating_point_v<T>) {
        const T inf = std::numeric_limits<T>::infinity();
        const T nan = std::numeric_limits<T>::quiet_NaN();
        for (T value : {inf, -inf, nan}) {
            expectError(IsFinite(value), INVALID_ARGUMENT, "not finite");
        }
        EXPECT_TRUE(Positive(std::numeric_limits<T>::min()));
        EXPECT_TRUE(Positive(inf));
        expectError(Positive(-inf), ARGUMENT_OUT_OF_RANGE, "positive");
        expectError(Positive(nan), ARGUMENT_OUT_OF_RANGE, "positive");
        expectError(Positive(T{-0.0}), ARGUMENT_OUT_OF_RANGE, "positive");
    }
}

TEST(ValueConstraints, NotEmptySupportsStringsAndContainers) {
    expectError(NotEmpty(std::string{}), INVALID_VALUE, "not empty");
    expectError(NotEmpty(std::vector<int>{}), INVALID_VALUE, "not empty");
    expectError(NotEmpty(std::map<std::string, int>{}), INVALID_VALUE, "not empty");
    expectError(NotEmpty(std::array<int, 0>{}), INVALID_VALUE, "not empty");
    EXPECT_TRUE(NotEmpty(std::string{" "}));
    EXPECT_TRUE(NotEmpty(std::string(1, '\0')));
    EXPECT_TRUE(NotEmpty(std::vector<int>{0}));
    EXPECT_TRUE(NotEmpty((std::map<std::string, int>{{"key", 0}})));
    EXPECT_TRUE(NotEmpty((std::array<int, 1>{0})));
    EXPECT_EQ(NotEmpty.name(), "not empty");
    EXPECT_EQ(Positive.name(), "positive");
    EXPECT_EQ(IsFinite.name(), "is finite");
}

TEST(ValueConstraints, OneOfMatchesEveryMemberAndRejectsNonmembers) {
    constexpr auto constraint = OneOf<-2, 0, 3>;
    for (int value : {-2, 0, 3}) {
        EXPECT_TRUE(constraint(value));
    }
    expectError(constraint(1), ARGUMENT_OUT_OF_RANGE, "expects one of: -2, 0, 3");
    EXPECT_EQ(constraint.name(), "one of");
    EXPECT_EQ(constraint.description(), "expects one of: -2, 0, 3");
    EXPECT_TRUE((OneOf<1, 1>(1)));
    EXPECT_TRUE((OneOf<false, true>(true)));
    EXPECT_TRUE((OneOf<1, 2>(2.0)));
    expectError(OneOf<>(1), ARGUMENT_OUT_OF_RANGE, "one of");
}

enum class Choice { first, second, third };
struct Token {
    int value;
    constexpr bool operator==(const Token&) const = default;
};

TEST(ValueConstraints, OneOfSupportsUnformattableEnumsAndStructuralValues) {
    constexpr auto choices = OneOf<Choice::first, Choice::second>;
    EXPECT_TRUE(choices(Choice::first));
    EXPECT_TRUE(choices(Choice::second));
    expectError(choices(Choice::third), ARGUMENT_OUT_OF_RANGE, "expects one of a set of values");
    EXPECT_EQ(choices.description(), "expects one of a set of values");
    constexpr auto tokens = OneOf<Token{1}, Token{2}>;
    EXPECT_TRUE(tokens(Token{2}));
    expectError(tokens(Token{3}), ARGUMENT_OUT_OF_RANGE, "expects one of a set of values");
    constexpr auto mixed = OneOf<1, Choice::first>;
    EXPECT_EQ(mixed.description(), "expects one of a set of values");
}

TEST(ValueConstraints, StartsWithChecksExactCaseSensitivePrefix) {
    constexpr auto constraint = StartsWith<Str{"hello"}>;
    EXPECT_TRUE(constraint(std::string{"hello"}));
    EXPECT_TRUE(constraint(std::string{"hello world"}));
    for (const std::string str : {"", "hell", "Hello world", "xhello"}) {
        expectError(constraint(str), INVALID_ARGUMENT, "expects prefix \"hello\"");
    }
    EXPECT_EQ(constraint.name(), "starts with");
    EXPECT_EQ(constraint.description(), "expects prefix \"hello\"");
    EXPECT_TRUE(StartsWith<Str{""}>(std::string{}));
    EXPECT_TRUE(StartsWith<Str{""}>(std::string{"anything"}));
    EXPECT_TRUE(constraint(std::string{"hello\0world", 11}));
}

TEST(ValueConstraints, EndsWithChecksExactCaseSensitiveSuffix) {
    constexpr auto constraint = EndsWith<Str{"world"}>;
    EXPECT_TRUE(constraint(std::string{"world"}));
    EXPECT_TRUE(constraint(std::string{"hello world"}));
    for (const std::string str : {"", "orld", "hello World", "worldx"}) {
        expectError(constraint(str), INVALID_ARGUMENT, "expects postfix \"world\"");
    }
    EXPECT_EQ(constraint.name(), "ends with");
    EXPECT_EQ(constraint.description(), "expects postfix \"world\"");
    EXPECT_TRUE(EndsWith<Str{""}>(std::string{}));
    EXPECT_TRUE(EndsWith<Str{""}>(std::string{"anything"}));
    EXPECT_TRUE(constraint(std::string{"hello\0world", 11}));
}
} // namespace
