#include <gtest/gtest.h>
#include "struo/struo.hpp"

TEST(DummyTests, Suceeds) {
    EXPECT_EQ(struo::get_project_version(), std::string_view{"0.1"});
    EXPECT_TRUE(struo::is_debug_enabled());
    STRUO_CHECK(true, "true is false?");
    STRUO_DCHECK(true, "how is true false?");
}