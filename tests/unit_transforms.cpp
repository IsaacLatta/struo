#include <gtest/gtest.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <map>

#include "struo/struo.hpp"
#include "struo/transforms/transforms.hpp"

TEST(Transforms, EnsurePrefix) {
    std::string path { "path" };
    auto result = struo::AddPrefix<struo::Str{"/"}>(path);
    EXPECT_EQ(result, "/path");
}

TEST(Transforms, ToLower) {
    std::string str { "LOWER" };
    EXPECT_EQ(struo::ToLower(str), "lower");
}

TEST(Transforms, ToUpper) {
    std::string str { "UPPER" };
    EXPECT_EQ(struo::ToLower(str), "upper");
}

TEST(Transforms, RelativeTo) {
    std::string relative { "documents" };
    EXPECT_EQ(struo::RelativeTo<struo::Str{"/home/user"}>(relative), "/home/user/documents");

    std::filesystem::path path { "index.html" };
    EXPECT_EQ(struo::RelativeTo<struo::Str{"/srv/www/html"}>(path), std::filesystem::path{"/srv/www/html/index.html"});
}

TEST(Transforms, Trim) {
    std::string str {"aaaaaawordaa"};
    EXPECT_EQ(struo::Trim<'a'>(str), "word");

    std::string str_a { "            word word   "};
    EXPECT_EQ(struo::TrimWhitespace(str_a), "word word");
}
