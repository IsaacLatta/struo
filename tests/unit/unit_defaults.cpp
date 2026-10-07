#include <gtest/gtest.h>

#include <concepts>
#include <string_view>

#include "struo/defaults.hpp"
#include "scoped.hpp"

namespace {

using namespace struo;

enum class Mode { ACTIVE, IDLE };

TEST(Defaults, ValuePreservesNumericTypesAndValues) {
    static_assert(std::same_as<decltype(Value<17>()), int>);
    static_assert(std::same_as<decltype(Value<1.25>()), double>);
    EXPECT_EQ(Value<17>(), 17);
    EXPECT_EQ(Value<-3>(), -3);
    EXPECT_DOUBLE_EQ(Value<1.25>(), 1.25);
}

TEST(Defaults, ValueSupportsEnumsAndBooleans) {
    static_assert(std::same_as<decltype(Value<Mode::ACTIVE>()), Mode>);
    EXPECT_EQ(Value<Mode::ACTIVE>(), Mode::ACTIVE);
    EXPECT_TRUE(Value<true>());
    EXPECT_FALSE(Value<false>());
}

TEST(Defaults, ValueSupportsStructuralConstants) {
    constexpr auto value = Value<Str{"default"}>();
    EXPECT_EQ(std::string_view{value.string}, "default");
}

#if STRUO_PLATFORM_LINUX

TEST(Defaults, FromEnvReadsAnInteger) {
    testlib::ScopedEnvVariable env{"STRUO_TEST_DEFAULTS", "24"};
    const auto result = FromEnv<"STRUO_TEST_DEFAULTS">.operator()<int>();
    ASSERT_TRUE(result) << result.error().what();
    ASSERT_TRUE(result.value());
    EXPECT_EQ(*result.value(), 24);
}

TEST(Defaults, FromEnvPreservesZeroAndNegativeValues) {
    for (const int expected : {0, -7}) {
        testlib::ScopedEnvVariable env{"STRUO_TEST_DEFAULTS", std::to_string(expected)};
        const auto result = FromEnv<"STRUO_TEST_DEFAULTS">.operator()<int>();
        ASSERT_TRUE(result) << result.error().what();
        ASSERT_TRUE(result.value());
        EXPECT_EQ(*result.value(), expected);
    }
}

TEST(Defaults, FromEnvReadsAFloatingPointValue) {
    testlib::ScopedEnvVariable env{"STRUO_TEST_DEFAULTS", "1.25"};
    const auto result = FromEnv<"STRUO_TEST_DEFAULTS">.operator()<double>();
    ASSERT_TRUE(result) << result.error().what();
    ASSERT_TRUE(result.value());
    EXPECT_DOUBLE_EQ(*result.value(), 1.25);
}

TEST(Defaults, FromEnvRejectsInvalidAndOutOfRangeValues) {
    for (const auto text : {"", "invalid", "24tail", "999999999999999999999999999"}) {
        SCOPED_TRACE(text);
        testlib::ScopedEnvVariable env{"STRUO_TEST_DEFAULTS", text};
        const auto result = FromEnv<"STRUO_TEST_DEFAULTS">.operator()<int>();
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().code(), WRONG_TYPE);
    }
}

TEST(Defaults, FromEnvReportsAnUnsetVariable) {
    {
        testlib::ScopedEnvVariable env{"STRUO_TEST_DEFAULTS", "24"};
    }
    const auto result = FromEnv<"STRUO_TEST_DEFAULTS">.operator()<int>();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), KEY_NOT_FOUND);
}

#endif

} // namespace
