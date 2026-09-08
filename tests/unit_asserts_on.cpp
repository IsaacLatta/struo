#include <gtest/gtest.h>

#include "struo/struo.hpp"

TEST(Asserts, AssertsWhenEnabled) {
    EXPECT_EXIT({
        STRUO_ASSERT(false, "ruh roh raggy");
    },
    [](int exit_code) { return exit_code != 0; },
    ".*ruh roh raggy.*");
}
