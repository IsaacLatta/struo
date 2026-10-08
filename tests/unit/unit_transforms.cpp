#include "struo/transforms.hpp"
#include "struo/Field.hpp"
#include "struo/constraints.hpp"

#include <gtest/gtest.h>

#include <clocale>
#include <filesystem>
#include <string>

namespace {
using namespace struo;
namespace fs = std::filesystem;

TEST(Transforms, TrimHandlesBoundariesAndPreservesInteriorCharacters) {
    struct Case { std::string input; std::string expected; };
    for (const auto& [input, expected] : {
        Case{"", ""}, Case{"a", ""}, Case{"aaaa", ""},
        Case{"word", "word"}, Case{"aaaword", "word"},
        Case{"wordaaa", "word"}, Case{"aaawordaa", "word"},
        Case{"aaxaa", "x"}, Case{"aawordawordaa", "wordaword"},
        Case{std::string{"aaX\0Yaa", 7}, std::string{"X\0Y", 3}}
    }) {
        SCOPED_TRACE(input);
        EXPECT_EQ(Trim<'a'>(input), expected);
        if (input.find('\0') == std::string::npos) {
            EXPECT_EQ(Trim<'a'>(fs::path{input}), fs::path{expected});
        }
    }
    EXPECT_EQ(Trim<'\0'>(std::string{"\0word\0", 6}), "word");
}

TEST(Transforms, CaseTransformsSupportStringsAndPaths) {
    EXPECT_EQ(ToLower(std::string{"MiXeD Az09_-/ ."}), "mixed az09_-/ .");
    EXPECT_EQ(ToUpper(std::string{"MiXeD Az09_-/ ."}), "MIXED AZ09_-/ .");
    EXPECT_EQ(ToLower(std::string{}), "");
    EXPECT_EQ(ToUpper(std::string{}), "");
    EXPECT_EQ(ToLower(fs::path{"Dir/MiXeD.txt"}), fs::path{"dir/mixed.txt"});
    EXPECT_EQ(ToUpper(fs::path{"Dir/MiXeD.txt"}), fs::path{"DIR/MIXED.TXT"});
    EXPECT_EQ(ToLower(fs::path{}), fs::path{});
    EXPECT_EQ(ToUpper(fs::path{}), fs::path{});
    EXPECT_EQ(ToLower(std::string{"A\0Z", 3}), (std::string{"a\0z", 3}));
    EXPECT_EQ(ToUpper(std::string{"a\0z", 3}), (std::string{"A\0Z", 3}));
}

TEST(Transforms, TrimWhitespaceHandlesAllStandardWhitespace) {
    const std::string whitespace{" \t\n\r\f\v"};

    struct Case {
        std::string input;
        std::string expected;
    };

    const std::vector<Case> cases{
        {"", ""},
        {"word", "word"},
        {whitespace, ""},
        {whitespace + "word", "word"},
        {"word" + whitespace, "word"},
        {whitespace + "word" + whitespace, "word"},
        {"\n \tword \r\n", "word"},
        {whitespace + "first" + whitespace + "second" + whitespace,
        "first" + whitespace + "second"},
        {whitespace + std::string{"a\0b", 3} + whitespace,
        std::string{"a\0b", 3}}
    };

    for (const auto& [input, expected] : cases) {
        SCOPED_TRACE(::testing::PrintToString(input));
        EXPECT_EQ(TrimWhitespace(input), expected);
    }

    for (char ch : whitespace) {
        SCOPED_TRACE(static_cast<int>(ch));
        const std::string input = std::string(2, ch) + "dir/file" + ch;
        EXPECT_EQ(TrimWhitespace(input), "dir/file");
        EXPECT_EQ(TrimWhitespace(fs::path{input}), fs::path{"dir/file"});
    }

    EXPECT_EQ(TrimWhitespace(fs::path{whitespace + "dir/file" + whitespace}), fs::path{"dir/file"});
    EXPECT_EQ(TrimWhitespace(fs::path{whitespace}), fs::path{});
    EXPECT_EQ(TrimWhitespace(fs::path{}), fs::path{});
}

TEST(Transforms, CaseTransformsPreserveHighBytesInCLocale) {
    const std::string previousLocale = std::setlocale(LC_CTYPE, nullptr);
    std::setlocale(LC_CTYPE, "C");
    std::string input;
    for (unsigned int byte = 128; byte < 256; ++byte) {
        input.push_back(static_cast<char>(byte));
    }
    EXPECT_EQ(ToLower(input), input);
    EXPECT_EQ(ToUpper(input), input);
    std::setlocale(LC_CTYPE, previousLocale.c_str());
}

TEST(Transforms, AddPrefixIsIdempotentAndPreservesEmptyInputs) {
    constexpr auto prefix = AddPrefix<Str{"pre-"}>;
    EXPECT_EQ(prefix(std::string{"value"}), "pre-value");
    EXPECT_EQ(prefix(std::string{"pre-value"}), "pre-value");
    EXPECT_EQ(prefix(std::string{"pre-"}), "pre-");
    EXPECT_EQ(prefix(std::string{"pre"}), "pre-pre");
    EXPECT_EQ(prefix(std::string{"PRE-value"}), "pre-PRE-value");
    EXPECT_EQ(prefix(std::string{}), "");
    EXPECT_EQ(prefix(prefix(std::string{"value"})), "pre-value");
    EXPECT_EQ(AddPrefix<Str{""}>(std::string{"value"}), "value");
    EXPECT_EQ(prefix(std::string{"x\0y", 3}), (std::string{"pre-x\0y", 7}));
    EXPECT_EQ(AddLeadingSlash(std::string{"dir/file"}), "/dir/file");
    EXPECT_EQ(AddLeadingSlash(std::string{"/dir/file"}), "/dir/file");
    EXPECT_EQ(AddLeadingSlash(std::string{}), "");
}

TEST(Transforms, RelativeToJoinsPathsWithoutFilesystemAccessOrNormalization) {
    constexpr auto relative = RelativeTo<Str{"/base"}>;
    EXPECT_EQ(relative(std::string{"dir/file"}), "/base/dir/file");
    EXPECT_EQ(relative(fs::path{"dir/file"}), fs::path{"/base/dir/file"});
    EXPECT_EQ(relative(std::string{"/absolute/file"}), "/absolute/file");
    EXPECT_EQ(relative(fs::path{"/absolute/file"}), fs::path{"/absolute/file"});
    EXPECT_EQ(relative(std::string{}), (fs::path{"/base"} / fs::path{}).string());
    EXPECT_EQ(relative(fs::path{}), fs::path{"/base"} / fs::path{});
    EXPECT_EQ(relative(fs::path{"../file"}), fs::path{"/base/../file"});
    EXPECT_EQ(RelativeTo<Str{"base"}>(fs::path{"file"}), fs::path{"base/file"});
    EXPECT_EQ(RelativeTo<Str{""}>(std::string{"file"}), "file");
}

TEST(Transforms, PreserveInputValues) {
    const std::string input{"  MiXeD  "};
    const std::string original = input;
    (void)TrimWhitespace(input);
    (void)ToLower(input);
    (void)ToUpper(input);
    (void)AddLeadingSlash(input);
    (void)RelativeTo<Str{"/base"}>(input);
    EXPECT_EQ(input, original);
    const fs::path path{input};
    const fs::path originalPath = path;
    (void)TrimWhitespace(path);
    (void)ToLower(path);
    (void)ToUpper(path);
    (void)RelativeTo<Str{"/base"}>(path);
    EXPECT_EQ(path, originalPath);
}
} // namespace

namespace keyword_test {
struct Config { int value{}; };
struct Threshold { int value; };
struct Constraint {
    int threshold;
    struo::Result<void> operator()(const int& value) const {
        return value >= threshold ? struo::ok()
            : struo::err(struo::INVALID_VALUE, "below threshold");
    }
};
struct Transform {
    int increment;
    int operator()(const int& value) const { return value + increment; }
};
}

namespace struo::detail {
template<>
struct KeywordTraits<ConstraintOperation, keyword_test::Threshold> {
    using callable_type = keyword_test::Constraint;
    static constexpr callable_type adapt(keyword_test::Threshold keyword) {
        return {keyword.value};
    }
};
template<>
struct KeywordTraits<TransformOperation, keyword_test::Threshold> {
    using callable_type = keyword_test::Transform;
    static constexpr callable_type adapt(keyword_test::Threshold keyword) {
        return {keyword.value};
    }
};
}

namespace {
TEST(Keywords, FieldAdaptsDescriptorAccordingToOperation) {
    using namespace struo;
    Field<&keyword_test::Config::value> field{
        Keys{"value"},
        Constraints{keyword_test::Threshold{5}},
        Transforms{keyword_test::Threshold{5}}
    };
    const auto constraints = field.getConstraints();
    EXPECT_FALSE(constraints.front()(4));
    EXPECT_TRUE(constraints.front()(5));
    const auto result = field.getTransforms().front()(4);
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 9);
}

TEST(Keywords, FieldAdaptsNestedIfPresentDescriptors) {
    using namespace struo;
    struct Config { std::optional<std::optional<std::string>> value; };
    Field<&Config::value> field{
        Keys{"value"}, Transforms{IfPresent<IfPresent<ToLower>>}
    };
    using adapted_type = decltype(
        detail::adapter_of<detail::TransformOperation,
            IfPresent<IfPresent<ToLower>>>());
    using expected_type = detail::IfPresentTransformT<
        detail::IfPresentTransformT<ToLower>{}>;
    static_assert(std::same_as<adapted_type, expected_type>);
    const auto& transform = field.getTransforms().front();
    auto present = transform(std::optional<std::optional<std::string>>{
        std::in_place, "MiXeD"});
    ASSERT_TRUE(present);
    ASSERT_TRUE(*present);
    EXPECT_EQ(**present, std::optional<std::string>{"mixed"});

    auto absent = transform(std::nullopt);
    ASSERT_TRUE(absent);
    EXPECT_FALSE(*absent);

    auto inner_absent = transform(std::optional<std::optional<std::string>>{
        std::in_place, std::nullopt});
    ASSERT_TRUE(inner_absent);
    ASSERT_TRUE(*inner_absent);
    EXPECT_FALSE(**inner_absent);
}

TEST(Keywords, OrdinaryMutableCallablesKeepTheirState) {
    using namespace struo;
    Field<&keyword_test::Config::value> field{
        Keys{"value"}, Transforms{[calls = 0](int value) mutable {
            return value + ++calls;
        }}
    };
    const auto transforms = field.getTransforms();
    EXPECT_EQ(*transforms.front()(10), 11);
    EXPECT_EQ(*transforms.front()(10), 12);
}
}

namespace {
TEST(Transforms, IfPresentChainsPlainAndResultTransformsThroughField) {
    using namespace struo;
    struct Config { std::optional<std::string> value; };
    constexpr auto suffix = [](const std::string& value) -> Result<std::string> {
        return value + "!";
    };
    Field<&Config::value> field{
        Keys{"value"}, Transforms{IfPresent<TrimWhitespace, ToLower, suffix>}
    };
    const std::optional<std::string> input{"  MiXeD  "};
    auto result = field.getTransforms().front()(input);
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, std::optional<std::string>{"mixed!"});
    EXPECT_EQ(input, std::optional<std::string>{"  MiXeD  "});
}

TEST(Transforms, IfPresentSkipsAbsentValuesAndStopsAtFirstError) {
    using namespace struo;
    static int before_calls;
    static int after_calls;
    before_calls = after_calls = 0;
    constexpr auto fail = [](const int&) -> Result<int> {
        ++before_calls;
        return err(INVALID_VALUE, "transform failed");
    };
    constexpr auto after = [](const int& value) {
        ++after_calls;
        return value + 1;
    };
    detail::IfPresentTransformT<fail, after> transform;
    auto absent = transform(std::optional<int>{});
    ASSERT_TRUE(absent);
    EXPECT_FALSE(*absent);
    EXPECT_EQ(before_calls, 0);
    EXPECT_EQ(after_calls, 0);

    auto failed = transform(std::optional<int>{7});
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error().code(), INVALID_VALUE);
    EXPECT_EQ(failed.error().what(), "transform failed");
    EXPECT_EQ(before_calls, 1);
    EXPECT_EQ(after_calls, 0);
}

TEST(Transforms, IfPresentPreservesEmptyStringsAndSupportsEmptyPacks) {
    using namespace struo;
    auto empty = detail::IfPresentTransformT<TrimWhitespace>{}(
        std::optional<std::string>{"   "});
    ASSERT_TRUE(empty);
    ASSERT_TRUE(*empty);
    EXPECT_TRUE((*empty)->empty());

    auto unchanged = detail::IfPresentTransformT<>{}(std::optional<int>{42});
    ASSERT_TRUE(unchanged);
    EXPECT_EQ(*unchanged, std::optional<int>{42});
}


}

namespace {
TEST(Keywords, ForEachConstraintWorksThroughFieldAndNestedDescriptors) {
    using namespace struo;
    struct Config { std::vector<std::vector<std::string>> value; };
    Field<&Config::value> field{
        Keys{"value"}, Constraints{ForEach<ForEach<NotEmpty>>}
    };
    EXPECT_TRUE(field.getConstraints().front()({{"first"}, {"second"}}));
    auto failed = field.getConstraints().front()({{"first"}, {""}});
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error().code(), INVALID_VALUE);
    EXPECT_NE(failed.error().what().find("index=1"), std::string_view::npos);
}

TEST(Keywords, ForEachTransformsSequencesAndComposesWithIfPresent) {
    using namespace struo;
    struct Config { std::optional<std::vector<std::string>> value; };
    Field<&Config::value> field{
        Keys{"value"}, Transforms{IfPresent<ForEach<TrimWhitespace, ToLower>>}
    };
    const auto& transform = field.getTransforms().front();
    const std::optional<std::vector<std::string>> input{{"  MiXeD  ", " WORD "}};
    auto result = transform(input);
    ASSERT_TRUE(result);
    ASSERT_TRUE(*result);
    EXPECT_EQ(**result, (std::vector<std::string>{"mixed", "word"}));
    EXPECT_EQ(*input, (std::vector<std::string>{"  MiXeD  ", " WORD "}));
    auto absent = transform(std::nullopt);
    ASSERT_TRUE(absent);
    EXPECT_FALSE(*absent);
    auto empty = transform(std::vector<std::string>{});
    ASSERT_TRUE(empty);
    ASSERT_TRUE(*empty);
    EXPECT_TRUE((*empty)->empty());
}

TEST(Transforms, ForEachStopsAtFirstFailureAndReportsIndex) {
    using namespace struo;
    static int calls;
    calls = 0;
    constexpr auto check = [](int value) -> Result<int> {
        ++calls;
        if(value == 2) return err(INVALID_VALUE, "rejected element");
        return value + 10;
    };
    constexpr auto after = [](int value) { return value * 2; };
    auto result = detail::ForEachTransformT<check, after>{}(std::vector<int>{1, 2, 3});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), INVALID_VALUE);
    EXPECT_EQ(result.error().what(),
        "\"for each\" transform failed: rejected element (on index=1)");
    EXPECT_EQ(calls, 2);
}
}
