#include <gtest/gtest.h>

#include <initializer_list>
#include <string_view>
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

struct NamedEven {
    static constexpr std::string_view name() { return "even"; }
    static std::string description() { return "must be even"; }
    Result<void> operator()(int value) const {
        if(auto result = IsEven(value); !result) {
            return err(result.error().code(), std::format("\"{}\" constraint failed: {}", name(), result.error().what()));
        }
        return ok();
    }
};

struct NamedOdd {
    static constexpr std::string_view name() { return "odd"; }
    Result<void> operator()(int value) const {
        if(auto result = IsOdd(value); !result) {
            return err(result.error().code(), std::format("\"{}\" constraint failed: {}", name(), result.error().what()));
        }
        return ok();
    }
};

TEST(CombinatorConstraintTest, MetadataHelpersNormalizeTypes) {
    constexpr NamedEven constraint;
    EXPECT_EQ(detail::name_of<decltype(constraint)>(), "even");
    EXPECT_EQ(detail::name_of<const volatile NamedEven&>(), "even");
    EXPECT_EQ(detail::description_of<decltype(constraint)>(), "must be even");
    EXPECT_EQ(detail::description_of<const volatile NamedEven&&>(), "must be even");
    EXPECT_EQ(detail::name_of<const int&>(), "<unnamed>");
    EXPECT_TRUE(detail::description_of<const int&>().empty());
}

TEST(CombinatorConstraintTest, DoesNotRepeatChildNames) {
    const auto result = Or<NamedEven{}>(3);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().what(),
        "\"or\" constraint failed: no constraints matched; \"even\" constraint failed: expected even");
}

TEST(CombinatorConstraintTest, ErrorsDescribeNamedConstraints) {
    const auto check_message = [](const Result<void>& result,
                                  std::initializer_list<std::string_view> fragments) {
        ASSERT_FALSE(result);
        for(const auto fragment : fragments) {
            EXPECT_NE(result.error().what().find(fragment), std::string_view::npos)
                << result.error().what();
        }
    };

    check_message(And<NamedEven{}, NamedOdd{}>(2),
        {"\"and\"", "\"odd\"", "expected odd"});
    check_message(Or<NamedEven{}, NamedEven{}>(3),
        {"\"or\"", "\"even\"", "expected even"});
    check_message(Not<NamedEven{}>(2),
        {"\"not\"", "\"even\"", "unexpectedly matched"});
    check_message(ExactlyOne<NamedEven{}, NamedEven{}>(3),
        {"\"exactly one\"", "no constraints matched", "\"even\"", "expected even"});
    check_message(ExactlyOne<NamedEven{}, NamedEven{}>(2),
        {"\"exactly one\"", "at least two", "\"even\""});
    const auto each = ForEach<NamedEven{}>(std::vector<int>{2, 3});
    check_message(each, {"\"for each\"", "\"even\"", "expected even", "index=1"});
    ASSERT_FALSE(each);
    EXPECT_EQ(each.error().code(), INVALID_VALUE);
    check_message(And<Not<NamedEven{}>>(2),
        {"\"and\"", "\"not\"", "\"even\""});
}

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
    EXPECT_TRUE((ForEach<Or<IsEven, IsOdd>>(std::vector<int>{1, 2, 3, 4})));
}

} // namespace
