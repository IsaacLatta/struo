#include <gtest/gtest.h>

#define STRUO_NO_ASSERT
#include "struo/struo.hpp"

TEST(Asserts, DoesNotAssertWhenDisabled) {
    STRUO_ASSERT(false, "this should have been compiled out!");
    SUCCEED();
}
