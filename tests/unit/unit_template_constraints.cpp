#include "struo/concepts.hpp"
#include "struo/constraints.hpp"
#include "struo/Field.hpp"
#include "struo/transforms.hpp"

#include <gtest/gtest.h>
#include <array>
#include <string>
#include <vector>

namespace {
using namespace struo;

struct WrongSize { std::string size() const; };
struct MutableSize { size_t size(); };
struct ConvertibleSize { int size() const; };
struct WrongEmpty { void empty() const; };
struct Unformattable {
    bool operator>(int) const;
};
struct LvalueConstraint {
    Result<void> operator()(const int&) & { return ok(); }
};
struct RvalueConstraint {
    Result<void> operator()(const int&) &&;
};
struct WrongResult {
    bool operator()(const int&) const;
};
struct ExplicitOnlyConstraint {
    template<typename T>
    requires std::same_as<T, int>
    Result<void> operator()(const auto&) { return ok(); }
};

static_assert(HasSizeApi<std::string>);
static_assert(HasSizeApi<std::vector<int>>);
static_assert(HasSizeApi<std::array<int, 3>>);
static_assert(!HasSizeApi<int>);
static_assert(!HasSizeApi<WrongSize>);
static_assert(!HasSizeApi<MutableSize>);
static_assert(!HasSizeApi<ConvertibleSize>);
static_assert(!HasEmptyApi<WrongEmpty>);
static_assert(IsConstraintFor<LvalueConstraint, int>);
static_assert(!IsConstraintFor<const LvalueConstraint, int>);
static_assert(!IsConstraintFor<RvalueConstraint, int>);
static_assert(!IsConstraintFor<WrongResult, int>);
static_assert(!IsConstraintFor<int, int>);
static_assert(HasFunctionSignature<LvalueConstraint, Result<void>(const int&)>);
static_assert(!HasFunctionSignature<LvalueConstraint, Result<void>(const std::string&)>);
static_assert(!HasFunctionSignature<int, Result<void>(int)>);

static_assert(IsConstraintFor<decltype(Positive), int>);
static_assert(!IsConstraintFor<decltype(Positive), std::string>);
static_assert(!IsConstraintFor<decltype(Positive), Unformattable>);
static_assert(IsConstraintFor<decltype(Range<0, 10>), double>);
static_assert(!IsConstraintFor<decltype(Range<0, 10>), std::string>);
static_assert(IsConstraintFor<decltype(SizeRange<0, 10>), std::vector<int>>);
static_assert(!IsConstraintFor<decltype(SizeRange<0, 10>), int>);
static_assert(!IsConstraintFor<decltype(SizeRange<0, 10>), WrongSize>);
static_assert(!IsConstraintFor<decltype(NotEmpty), WrongEmpty>);
static_assert(IsConstraintFor<decltype(OneOf<1, 2>), double>);
static_assert(!IsConstraintFor<decltype(OneOf<1, 2>), std::string>);
static_assert(!IsConstraintFor<decltype(And<Positive, StartsWith<"a">>), int>);
static_assert(!IsConstraintFor<decltype(Or<Positive, StartsWith<"a">>), int>);
static_assert(!IsConstraintFor<decltype(Not<StartsWith<"a">>), int>);
static_assert(!IsConstraintFor<decltype(ExactlyOne<Positive, StartsWith<"a">>), int>);
static_assert(IsConstraintFor<decltype(And<Positive, Or<Range<0, 10>, OneOf<20>>>), int>);

using Each = decltype(detail::adapter_of<detail::ConstraintOperation, ForEach<Positive>>());
using WrongEach = decltype(detail::adapter_of<detail::ConstraintOperation, ForEach<StartsWith<"a">>>());
static_assert(IsConstraintFor<Each, std::vector<int>>);
static_assert(!IsConstraintFor<Each, int>);
static_assert(!IsConstraintFor<WrongEach, std::vector<int>>);

template<auto Min, auto Max>
concept CanMakeRange = requires { Range<Min, Max>; };
template<size_t Min, size_t Max>
concept CanMakeSizeRange = requires { SizeRange<Min, Max>; };
template<auto Value>
concept CanMakeAtLeast = requires { AtLeast<Value>; };
template<auto Value>
concept CanMakeAtMost = requires { AtMost<Value>; };
static_assert(CanMakeRange<0, 10>);
static_assert(!CanMakeRange<10, 0>);
static_assert(!CanMakeRange<0, 10u>);
static_assert(!CanMakeRange<Str{"a"}, Str{"b"}>);
static_assert(!CanMakeSizeRange<10, 0>);
static_assert(CanMakeAtLeast<0> && CanMakeAtMost<0>);
static_assert(!CanMakeAtLeast<Str{"a"}> && !CanMakeAtMost<Str{"a"}>);

struct Config { int number; std::vector<int> numbers; std::string text; std::optional<int> optional; };
using NumberField = Field<&Config::number>;
using NumbersField = Field<&Config::numbers>;
template<typename F, typename Arg>
concept CanMakeField = requires(Arg arg) { F{Keys{"value"}, arg}; };
static_assert(CanMakeField<NumberField, decltype(Constraints{Positive})>);
static_assert(!CanMakeField<NumberField, decltype(Constraints{StartsWith<"a">})>);
static_assert(!CanMakeField<NumberField, decltype(Constraints{And<Positive, StartsWith<"a">>})>);
static_assert(!CanMakeField<NumberField, decltype(Constraints{WrongResult{}})>);
static_assert(!CanMakeField<NumberField, decltype(Constraints{RvalueConstraint{}})>);
static_assert(CanMakeField<NumberField, decltype(Constraints{LvalueConstraint{}})>);
static_assert(CanMakeField<NumberField, decltype(Constraints{ExplicitOnlyConstraint{}})>);
static_assert(!CanMakeField<Field<&Config::text>, decltype(Constraints{ExplicitOnlyConstraint{}})>);
static_assert(CanMakeField<NumbersField, decltype(Constraints{ForEach<Positive>})>);
static_assert(!CanMakeField<NumbersField, decltype(Constraints{ForEach<StartsWith<"a">>})>);
static_assert(!std::is_constructible_v<NumberField, Keys, decltype(Constraints{StartsWith<"a">})>);
static_assert(!std::is_constructible_v<NumberField, Keys, int>);
static_assert(!std::is_constructible_v<NumberField, Keys, Keys>);
static_assert(CanMakeField<NumberField, decltype(Defaults{FromEnv<"STRUO_TEST_DEFAULT">})>);
static_assert(!CanMakeField<NumberField, decltype(Defaults{[] { return std::string{}; }})>);
static_assert(!CanMakeField<NumberField, decltype(Transforms{ToLower})>);
static_assert(!CanMakeField<NumbersField, decltype(Transforms{ForEach<ToLower>})>);
static_assert(!CanMakeField<Field<&Config::optional>, decltype(Transforms{IfPresent<ToLower>})>);
static_assert(CanMakeField<Field<&Config::optional>, decltype(Transforms{IfPresent<[](int n) { return n + 1; }>})>);
static_assert(!IsConstraintFor<decltype(And<LvalueConstraint{}>), int>);

TEST(TemplateConstraints, WrapsLvalueAndExplicitTypeCallables) {
    NumberField field{Keys{"number"}, Constraints{LvalueConstraint{}, ExplicitOnlyConstraint{}}};
    for (const auto& constraint : field.getConstraints()) {
        EXPECT_TRUE(constraint(1));
    }
}

TEST(TemplateConstraints, AcceptsAdaptedConstraintsAndTransforms) {
    NumbersField field{Keys{"numbers"}, Constraints{ForEach<And<Positive, Range<0, 10>>>},
        Transforms{ForEach<[](const int& value) { return value + 1; }>} };
    EXPECT_TRUE(field.getConstraints().front()(std::vector<int>{1, 2}));
    EXPECT_FALSE(field.getConstraints().front()(std::vector<int>{1, -2}));
    const auto result = field.getTransforms().front()(std::vector<int>{1, 2});
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, (std::vector<int>{2, 3}));
}
} // namespace
