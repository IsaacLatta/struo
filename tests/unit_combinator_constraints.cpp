#include <gtest/gtest.h>

#include <vector>

#include "struo/struo.hpp"

namespace {

using namespace struo;

constexpr auto IsEven = [](int value) -> Result<void> {
    if (value % 2 == 0) {
        return ok();
    }
    return err(INVALID_VALUE, "expected even");
};

constexpr auto IsOdd = [](int value) -> Result<void> {
    if (value % 2 != 0) {
        return ok();
    }
    return err(INVALID_VALUE, "expected odd");
};

TEST(CombinatorConstraintTest, AndRequiresAllToMatch) {
    EXPECT_TRUE((And<IsEven, IsEven>(2)));
    EXPECT_FALSE((And<IsEven, IsOdd>(2)));
    EXPECT_FALSE((And<IsEven, IsEven>(3)));
}

TEST(CombinatorConstraintTest, OrRequiresAtLeastOneToMatch) {
    EXPECT_TRUE((Or<IsEven, IsOdd>(2)));
    EXPECT_TRUE((Or<IsEven, IsOdd>(3)));
    EXPECT_TRUE((Or<IsEven, IsEven>(2)));
    EXPECT_FALSE((Or<IsEven, IsEven>(3)));
}

TEST(CombinatorConstraintTest, NotInvertsTheResult) {
    EXPECT_TRUE(Not<IsEven>(3));
    EXPECT_FALSE(Not<IsEven>(2));
}

TEST(CombinatorConstraintTest, ExactlyOneRequiresOneMatch) {
    EXPECT_TRUE((ExactlyOne<IsEven, IsOdd>(2)));
    EXPECT_TRUE((ExactlyOne<IsEven, IsOdd>(3)));
    EXPECT_FALSE((ExactlyOne<IsEven, IsEven>(3)));
    EXPECT_FALSE((ExactlyOne<IsEven, IsEven>(2)));
    EXPECT_FALSE((ExactlyOne<IsEven, IsEven, IsEven>(2)));
}

TEST(CombinatorConstraintTest, SupportsNesting) {
    constexpr auto constraint = And<Or<IsEven, IsOdd>, Not<IsOdd>>;

    EXPECT_TRUE(constraint(2));
    EXPECT_FALSE(constraint(3));
}

TEST(CombinatorConstraintTest, ForEachRequiresEveryElementToMatch) {
    EXPECT_TRUE(ForEach<IsEven>(std::vector<int>{2, 4, 6}));
    EXPECT_FALSE(ForEach<IsEven>(std::vector<int>{2, 3, 6}));
}

TEST(CombinatorConstraintTest, ForEachAcceptsAnEmptyContainer) {
    EXPECT_TRUE(ForEach<IsEven>(std::vector<int>{}));
}

TEST(CombinatorConstraintTest, ForEachRequiresAllConstraintsToMatch) {
    EXPECT_TRUE((ForEach<IsEven, IsEven>(std::vector<int>{2, 4})));
    EXPECT_FALSE((ForEach<IsEven, IsOdd>(std::vector<int>{2, 4})));
}

TEST(CombinatorConstraintTest, ForEachSupportsNestedCombinators) {
    EXPECT_TRUE((ForEach<Or<IsEven, IsOdd>>(
        std::vector<int>{1, 2, 3, 4})));
}

} // namespace
