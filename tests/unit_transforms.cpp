#include "struo/transforms/transforms.hpp"

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

TEST(Transforms, TrimWhitespaceTrimsSpacesAndPreservesOtherCharacters) {
    EXPECT_EQ(TrimWhitespace(std::string{"   word word   "}), "word word");
    EXPECT_EQ(TrimWhitespace(std::string{"   "}), "");
    EXPECT_EQ(TrimWhitespace(std::string{}), "");
    // The current alias is Trim<' '>, so tabs and newlines are preserved.
    EXPECT_EQ(TrimWhitespace(std::string{" \tword\n "}), "\tword\n");
    EXPECT_EQ(TrimWhitespace(fs::path{"  dir/file  "}), fs::path{"dir/file"});
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
