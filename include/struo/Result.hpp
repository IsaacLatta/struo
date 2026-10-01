#pragma once

#include <concepts>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "struo/forward.hpp"
#include "struo/detail/asserts.hpp"

namespace struo {

enum class ErrorCode {
    UNKNOWN_ERROR,
    FILE_NOT_FOUND,
    OPEN_FILE_FAILED,
    READ_FILE_FAILED,
    INVALID_ARGUMENT,
    WRONG_TYPE,
    KEY_NOT_FOUND,
    PARSE_ERROR,
    INVALID_VALUE,
    SYNTAX_ERROR,
    ARGUMENT_OUT_OF_RANGE,
    NOT_IMPLEMENTED
}; using enum ErrorCode;

class Error {
public:
    constexpr explicit Error(ErrorCode ec, std::string message, std::source_location where = std::source_location::current())
        : message_(std::move(message)), location_(where), code_(ec) {}

    [[nodiscard]] constexpr std::string_view what() const noexcept {
        return message_;
    }

    [[nodiscard]] constexpr std::source_location where() const noexcept {
        return location_;
    }

    [[nodiscard]] constexpr ErrorCode code() const noexcept {
        return code_;
    }

private:
    std::string message_{};
    std::source_location location_{};
    ErrorCode code_{};
};

template<typename T>
requires (!std::is_reference_v<T> && !std::same_as<std::remove_cvref_t<T>, Error>)
class [[nodiscard]] Result {
public:
    constexpr Result(T&& t) noexcept(std::is_nothrow_move_constructible_v<T>) : storage_(std::in_place_type<T>, std::move(t)) {}
    constexpr Result(const T& t) noexcept(std::is_nothrow_constructible_v<T>) : storage_(std::in_place_type<T>, t) {}

    constexpr Result(Error&& error) noexcept(std::is_nothrow_move_constructible_v<Error>): storage_(std::in_place_type<Error>, std::move(error)) {}
    constexpr Result(const Error& error) noexcept(std::is_nothrow_constructible_v<Error>) : storage_(std::in_place_type<Error>, error) {}

    [[nodiscard]] constexpr bool ok() const noexcept {
        return hasValue();
    }

    [[nodiscard]] constexpr bool hasValue() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    [[nodiscard]] constexpr const T& value() const & {
        STRUO_ASSERT(ok(), "failed to check: !struo::Result");
        return std::get<0>(storage_);
    }

    [[nodiscard]] constexpr T& value() & {
        STRUO_ASSERT(ok(), "failed to check: !struo::Result");
        return std::get<0>(storage_);
    }

    [[nodiscard]] constexpr T&& value() && {
        STRUO_ASSERT(ok(), "failed to check: !struo::Result");
        return std::get<0>(std::move(storage_));
    }

    [[nodiscard]] constexpr const Error& error() const & {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return std::get<1>(storage_);
    }

    [[nodiscard]] constexpr Error& error() & {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return std::get<1>(storage_);
    }

    [[nodiscard]] constexpr Error&& error() && {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return std::get<1>(std::move(storage_));
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return hasValue();
    }

    [[nodiscard]] constexpr T&& operator*() && {
        return std::move(*this).value();
    }

    [[nodiscard]] constexpr T& operator*() & {
        return value();
    }

    [[nodiscard]] constexpr const T& operator*() const & {
        return value();
    }

private:
    std::variant<T, Error> storage_{};
};

template<>
class [[nodiscard]] Result<void> {
public:
    constexpr Result() = default;
    constexpr ~Result() = default;

    constexpr Result(Result&&) = default;
    constexpr Result(const Result&) = default;

    constexpr Result& operator=(Result&&) = default;
    constexpr Result& operator=(const Result&) = default;

    constexpr Result(Error&& error) noexcept(std::is_nothrow_move_constructible_v<Error>) : error_(std::in_place, std::move(error)) {}
    constexpr Result(const Error& error) noexcept(std::is_nothrow_copy_constructible_v<Error>) : error_(std::in_place, error) {}

    [[nodiscard]] constexpr bool ok() const noexcept {
        return !error_.has_value();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return ok();
    }

    [[nodiscard]] constexpr const Error& error() const & {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return *error_;
    }

    [[nodiscard]] constexpr Error& error() & {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return *error_;
    }

    [[nodiscard]] constexpr Error&& error() && {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return std::move(*error_);
    }

private:
    std::optional<Error> error_{};
};

template<typename T>
[[nodiscard]] constexpr Error err(Result<T>& result) {
    return std::move(result).error();
}

template<typename... Args>
requires std::is_constructible_v<Error, Args...>
[[nodiscard]] constexpr auto err(Args&&... args) {
    return Error{std::forward<Args>(args)...};
}

[[nodiscard]] constexpr auto ok() {
    return Result<void>{};
}

}
