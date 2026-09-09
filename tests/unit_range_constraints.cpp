#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <vector>

#include "struo/struo.hpp"

namespace {

using namespace struo;

template<typename T>
class RangeConstraintTest : public ::testing::Test {};

using NumericTypes = ::testing::Types<int, long long, unsigned int, size_t, float, double>;

TYPED_TEST_SUITE(RangeConstraintTest, NumericTypes);

TYPED_TEST(RangeConstraintTest, AcceptsValuesWithinInclusiveRange) {
    using T = TypeParam;

    constexpr T min = static_cast<T>(10);
    constexpr T max = static_cast<T>(20);

    EXPECT_TRUE((Range<min, max>(static_cast<T>(10))));
    EXPECT_TRUE((Range<min, max>(static_cast<T>(15))));
    EXPECT_TRUE((Range<min, max>(static_cast<T>(20))));
}

TYPED_TEST(RangeConstraintTest, RejectsValuesOutsideRange) {
    using T = TypeParam;

    constexpr T min = static_cast<T>(10);
    constexpr T max = static_cast<T>(20);

    {
        auto result = Range<min, max>(static_cast<T>(9));

        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().code(), ARGUMENT_OUT_OF_RANGE);
    }

    {
        auto result = Range<min, max>(static_cast<T>(21));

        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().code(), ARGUMENT_OUT_OF_RANGE);
    }
}

TYPED_TEST(RangeConstraintTest, SupportsPositiveAndNegativeRanges) {
    using T = TypeParam;

    constexpr T positive_min = static_cast<T>(1);
    constexpr T positive_max = static_cast<T>(10);

    EXPECT_TRUE((Range<positive_min, positive_max>(static_cast<T>(5))));

    if constexpr (std::is_signed_v<T> || std::is_floating_point_v<T>) {
        constexpr T negative_min = static_cast<T>(-10);
        constexpr T negative_max = static_cast<T>(-1);

        EXPECT_TRUE(
            (Range<negative_min, negative_max>(static_cast<T>(-5)))
        );

        EXPECT_FALSE(
            (Range<negative_min, negative_max>(static_cast<T>(5)))
        );
    }
}

TYPED_TEST(RangeConstraintTest, GreaterThanIsInclusive) {
    using T = TypeParam;

    constexpr T min = static_cast<T>(10);

    EXPECT_FALSE(AtMost<min>(static_cast<T>(9)));
    EXPECT_TRUE(AtMost<min>(static_cast<T>(10)));
    EXPECT_TRUE(AtMost<min>(static_cast<T>(11)));
}

TYPED_TEST(RangeConstraintTest, LessThanIsInclusive) {
    using T = TypeParam;

    constexpr T max = static_cast<T>(10);

    EXPECT_TRUE(AtLeast<max>(static_cast<T>(9)));
    EXPECT_TRUE(AtLeast<max>(static_cast<T>(10)));
    EXPECT_FALSE(AtLeast<max>(static_cast<T>(11)));

    if constexpr (std::is_signed_v<T> || std::is_floating_point_v<T>) {
        EXPECT_TRUE(AtLeast<max>(static_cast<T>(-10)));
    }
}

TYPED_TEST(RangeConstraintTest, SupportsFractionalRanges) {
    using T = TypeParam;

    if constexpr (std::is_floating_point_v<T>) {
        constexpr T min = static_cast<T>(-1.5);
        constexpr T max = static_cast<T>(2.5);

        EXPECT_TRUE((Range<min, max>(static_cast<T>(-1.25))));
        EXPECT_TRUE((Range<min, max>(static_cast<T>(0.5))));
        EXPECT_TRUE((Range<min, max>(static_cast<T>(2.25))));
        EXPECT_FALSE((Range<min, max>(static_cast<T>(-2.0))));
        EXPECT_FALSE((Range<min, max>(static_cast<T>(3.0))));
    }
}

} // namespace
