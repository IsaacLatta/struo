#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "struo/struo.hpp"

namespace {

using namespace struo;

struct CopyVerseMove {
    static inline uint32_t copies = 0;
    static inline uint32_t moves = 0;

    CopyVerseMove() = default;

    CopyVerseMove(const CopyVerseMove&) {
        ++copies;
    }

    CopyVerseMove(CopyVerseMove&&) noexcept {
        ++moves;
    }

    static void reset() {
        copies = 0;
        moves = 0;
    }
};

struct MoveOnly {
    explicit MoveOnly(int value) : value(value) {}

    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    MoveOnly(MoveOnly&& other) noexcept : value(other.value) {
        other.value = -1;
    }

    MoveOnly& operator=(MoveOnly&& other) noexcept {
        value = other.value;
        other.value = -1;
        return *this;
    }

    int value{};
};

TEST(Result, HoldsValue) {
    Result<int> result{42};

    EXPECT_TRUE(result);
    EXPECT_TRUE(result.ok());
    ASSERT_TRUE(result.hasValue());
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(*result, 42);
}

TEST(Result, HoldsError) {
    Result<int> result{
        err(INVALID_VALUE, "invalid value")
    };

    EXPECT_FALSE(result);
    ASSERT_FALSE(result.ok());
    ASSERT_FALSE(result.hasValue());

    EXPECT_EQ(result.error().code(), INVALID_VALUE);
    EXPECT_EQ(result.error().what(), "invalid value");
}

TEST(Result, ProvidesMutableValueReference) {
    Result<int> result{42};

    result.value() = 100;

    EXPECT_EQ(result.value(), 100);

    *result = 200;

    EXPECT_EQ(result.value(), 200);
}

TEST(Result, ProvidesConstValueReference) {
    const Result<int> result{42};

    static_assert(std::same_as<decltype(result.value()), const int&>);
    static_assert(std::same_as<decltype(*result), const int&>);

    EXPECT_EQ(result.value(), 42);
}

TEST(Result, SupportsMoveOnlyValues) {
    Result<std::unique_ptr<int>> result{ std::make_unique<int>(42) };

    ASSERT_TRUE(result);
    ASSERT_NE(result.value(), nullptr);
    EXPECT_EQ(*result.value(), 42);

    auto ptr = std::move(result).value();

    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 42);
}

TEST(Result, RvalueValueReturnsRvalueReference) {
    static_assert(std::same_as<decltype(std::declval<Result<MoveOnly>&&>().value()), MoveOnly&&>);

    Result<MoveOnly> result{ MoveOnly{42} };

    MoveOnly moved = std::move(result).value();

    EXPECT_EQ(moved.value, 42);

    // The MoveProbe inside Result was moved from.
    EXPECT_EQ(result.value().value, -1);
}

TEST(Result, MovesValueFromLvalueResult) {
    Result<MoveOnly> result{MoveOnly{42}};

    MoveOnly moved = std::move(result.value());

    EXPECT_EQ(moved.value, 42);
    EXPECT_EQ(result.value().value, -1);
}

Result<int> function_returning_value() {
    return 42;
}

TEST(Result, FunctionReturningValue) {
    Result<int> result = function_returning_value();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.hasValue());

    ASSERT_EQ(result.value(), 42);
}


TEST(Result, RvalueDereferenceReturnsRvalueReference) {
    static_assert(std::same_as<decltype(*std::declval<Result<MoveOnly>&&>()), MoveOnly&&>);

    Result<MoveOnly> result{ MoveOnly{42} };

    MoveOnly moved = *std::move(result);

    EXPECT_EQ(moved.value, 42);
    EXPECT_EQ(result.value().value, -1);
}

TEST(Result, ProvidesErrorReferences) {
    Result<int> result{ err(INVALID_ARGUMENT, "bad argument") };

    static_assert(std::same_as<decltype(result.error()), Error&>);

    const auto& const_result = result;

    static_assert(std::same_as<decltype(const_result.error()), const Error&>);

    static_assert(std::same_as<decltype(std::move(result).error()), Error&&>);

    EXPECT_EQ(result.error().code(), INVALID_ARGUMENT);
}

TEST(ResultVoid, DefaultsToSuccess) {
    Result<void> result{};

    EXPECT_TRUE(result);
    EXPECT_TRUE(result.ok());
}

Result<CopyVerseMove> return_local_value() {
    CopyVerseMove value;
    return value;
}

TEST(Result, ReturningLocalValueUsesRvalueConstructor) {
    CopyVerseMove::reset();

    auto result = return_local_value();

    ASSERT_TRUE(result);
    EXPECT_EQ(CopyVerseMove::copies, 0);
    EXPECT_EQ(CopyVerseMove::moves, 1);
}

TEST(ResultVoid, HoldsError) {
    Result<void> result{
        err(PARSE_ERROR, "failed to parse")
    };

    EXPECT_FALSE(result);
    EXPECT_FALSE(result.ok());

    EXPECT_EQ(result.error().code(), PARSE_ERROR);
    EXPECT_EQ(result.error().what(), "failed to parse");
}

TEST(ResultHelpers, OkCreatesSuccessfulVoidResult) {
    auto result = ok();

    EXPECT_TRUE(result);
    EXPECT_TRUE(result.ok());
}

Result<void> function_returning_error() {
    return err(INVALID_VALUE, "bad value");
}

TEST(ResultHelpers, ErrorConvertsToVoidResult) {
    auto result = function_returning_error();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), INVALID_VALUE);
    EXPECT_EQ(result.error().what(), "bad value");
}

} // namespace