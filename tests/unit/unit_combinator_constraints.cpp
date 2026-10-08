#include <gtest/gtest.h>

#include <initializer_list>
#include <optional>
#include <source_location>
#include <string_view>
#include <utility>
#include <vector>

#include "struo/Field.hpp"
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
    EXPECT_TRUE((detail::description_of<const int&>().empty()));
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
    const auto each = detail::adapter_of<detail::ConstraintOperation, ForEach<NamedEven{}>>()(std::vector<int>{2, 3});
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
    EXPECT_TRUE((detail::adapter_of<detail::ConstraintOperation, ForEach<IsEven>>()(std::vector<int>{2, 4, 6})));
    EXPECT_FALSE((detail::adapter_of<detail::ConstraintOperation, ForEach<IsEven>>()(std::vector<int>{2, 3, 6})));
}

TEST(CombinatorConstraintTest, ForEachAcceptsAnEmptyContainer) {
    EXPECT_TRUE((detail::adapter_of<detail::ConstraintOperation, ForEach<IsEven>>()(std::vector<int>{})));
}

TEST(CombinatorConstraintTest, ForEachRequiresAllConstraintsToMatch) {
    EXPECT_TRUE((detail::adapter_of<detail::ConstraintOperation, ForEach<IsEven, IsEven>>()(std::vector<int>{2, 4})));
    EXPECT_FALSE((detail::adapter_of<detail::ConstraintOperation, ForEach<IsEven, IsOdd>>()(std::vector<int>{2, 4})));
}

TEST(CombinatorConstraintTest, ForEachSupportsNestedCombinators) {
    EXPECT_TRUE((detail::adapter_of<detail::ConstraintOperation, ForEach<Or<IsEven, IsOdd>>>()(std::vector<int>{1, 2, 3, 4})));
}

TEST(IfPresentConstraintTest, AbsentValueSkipsAllConstraints) {
    static int calls;
    calls = 0;
    constexpr auto fail = [](int) -> Result<void> {
        ++calls;
        return err(INVALID_VALUE, "must not be invoked");
    };
    struct Config { std::optional<int> value; };
    Field<&Config::value> field{Keys{"value"}, Constraints{IfPresent<fail, fail>}};

    EXPECT_TRUE(field.getConstraints().front()(std::nullopt));
    EXPECT_EQ(calls, 0);
}

TEST(IfPresentConstraintTest, PresentValueInvokesAllConstraintsInOrder) {
    static std::vector<std::pair<int, int>> calls;
    calls.clear();
    constexpr auto first = [](int value) -> Result<void> {
        calls.emplace_back(1, value);
        return ok();
    };
    constexpr auto second = [](int value) -> Result<void> {
        calls.emplace_back(2, value);
        return ok();
    };
    constexpr auto third = [](int value) -> Result<void> {
        calls.emplace_back(3, value);
        return ok();
    };
    struct Config { std::optional<int> value; };
    Field<&Config::value> field{
        Keys{"value"}, Constraints{IfPresent<first, second, third>}
    };

    EXPECT_TRUE(field.getConstraints().front()(std::optional<int>{42}));
    const std::vector<std::pair<int, int>> expected{{1, 42}, {2, 42}, {3, 42}};
    EXPECT_EQ(calls, expected);
}

TEST(IfPresentConstraintTest, StopsAtFirstFailureAndPreservesError) {
    static std::vector<int> calls;
    static const Error expected{INVALID_VALUE, "second constraint failed", std::source_location::current()};
    constexpr auto first = [](int) -> Result<void> {
        calls.push_back(1);
        return ok();
    };
    constexpr auto second = [](int) -> Result<void> {
        calls.push_back(2);
        return expected;
    };
    constexpr auto third = [](int) -> Result<void> {
        calls.push_back(3);
        return err(WRONG_TYPE, "third constraint failed");
    };

    // Also check that a failure in the first position skips every later check.
    const auto check = [&]<auto... Checks>() {
        calls.clear();
        struct Config { std::optional<int> value; };
        Field<&Config::value> field{Keys{"value"}, Constraints{IfPresent<Checks...>}};
        const auto result = field.getConstraints().front()(std::optional<int>{42});

        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().code(), expected.code());
        EXPECT_EQ(result.error().what(), expected.what());
        EXPECT_EQ(result.error().where().file_name(), std::string_view{expected.where().file_name()});
        EXPECT_EQ(result.error().where().function_name(), std::string_view{expected.where().function_name()});
        EXPECT_EQ(result.error().where().line(), expected.where().line());
        EXPECT_EQ(result.error().where().column(), expected.where().column());
    };

    check.template operator()<first, second, third>();
    EXPECT_EQ(calls, (std::vector<int>{1, 2}));
    check.template operator()<second, third>();
    EXPECT_EQ(calls, (std::vector<int>{2}));
}

TEST(IfPresentConstraintTest, EmptyConstraintPackSucceeds) {
    const detail::IfPresentConstraintT<> constraint;
    EXPECT_TRUE(constraint(std::optional<int>{}));
    EXPECT_TRUE(constraint(std::optional<int>{42}));
}

TEST(IfPresentConstraintTest, SupportsNestedDescriptorsThroughField) {
    struct Config {
        std::optional<std::vector<int>> list;
        std::vector<std::optional<int>> elements;
        std::optional<std::optional<int>> nested;
    };
    Field<&Config::list> list{
        Keys{"list"}, Constraints{IfPresent<ForEach<IsEven>>}
    };
    const auto& checkList = list.getConstraints().front();
    EXPECT_TRUE(checkList(std::nullopt));
    EXPECT_TRUE(checkList(std::vector<int>{2, 4}));
    EXPECT_FALSE(checkList(std::vector<int>{2, 3}));

    Field<&Config::elements> elements{
        Keys{"elements"}, Constraints{ForEach<IfPresent<IsEven>>}
    };
    const auto& checkElements = elements.getConstraints().front();
    EXPECT_TRUE(checkElements({std::nullopt, 2}));
    EXPECT_FALSE(checkElements({std::nullopt, 3}));

    Field<&Config::nested> nested{
        Keys{"nested"}, Constraints{IfPresent<IfPresent<IsEven>>}
    };
    const auto& checkNested = nested.getConstraints().front();
    EXPECT_TRUE(checkNested(std::nullopt));
    EXPECT_TRUE(checkNested(std::optional<std::optional<int>>{std::in_place, std::nullopt}));
    EXPECT_TRUE(checkNested(std::optional<std::optional<int>>{std::in_place, 2}));
    EXPECT_FALSE(checkNested(std::optional<std::optional<int>>{std::in_place, 3}));
}

} // namespace
