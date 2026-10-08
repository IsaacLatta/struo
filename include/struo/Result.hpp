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

/**
 * @brief Identifies the category of a Struo error.
 *
 * Use Error::code() to inspect the category and Error::what() for the
 * accompanying message. Enumerators are also available directly
 * in the struo namespace (e.g., struo::FILE_NOT_FOUND).
 */
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

/**
 * @brief Primary error interface containing an error category, diagnostic message, and source location.
 * Inspect what() for the diagnostic message, where() for the source location, and code() for the error category.
 */
class Error {
public:
    constexpr explicit Error(ErrorCode ec, std::string message, std::source_location where = std::source_location::current())
        : message_(std::move(message)), location_(where), code_(ec) {}

    /**
     * @return The diagnostic message (e.g., what happened).
     */
    [[nodiscard]] constexpr std::string_view what() const noexcept {
        return message_;
    }

    /**
     * @return The source location of where the error was constructed (e.g., where it happened).
     */
    [[nodiscard]] constexpr std::source_location where() const noexcept {
        return location_;
    }

    /**
     * @return The error category of the owning error.
     */
    [[nodiscard]] constexpr ErrorCode code() const noexcept {
        return code_;
    }

private:
    std::string message_{};
    std::source_location location_{};
    ErrorCode code_{};
};

/**
 * @brief Holds either a successful value or an Error.
 * @tparam T The owned value type; must not be a reference or Error.
 *
 * Check ok(), hasValue(), or the explicit boolean conversion before accessing
 * the value or error. Accessors return references to either the error or value.
 * See Result<void> for operations without a return value.
 * @code{.cpp}
 * struo::Result<int> result = checkedPort(8080);
 * if (result) {
 *     do_something(*result);
 * } else {
 *     handle_error(result.error());
 * }
 * @endcode
 */
template<typename T>
requires (!std::is_reference_v<T> && !std::same_as<std::remove_cvref_t<T>, Error>)
class [[nodiscard]] Result {
public:
    constexpr Result(T&& t) noexcept(std::is_nothrow_move_constructible_v<T>) : storage_(std::in_place_type<T>, std::move(t)) {}
    constexpr Result(const T& t) noexcept(std::is_nothrow_constructible_v<T>) : storage_(std::in_place_type<T>, t) {}
    constexpr Result(Error&& error) noexcept(std::is_nothrow_move_constructible_v<Error>): storage_(std::in_place_type<Error>, std::move(error)) {}
    constexpr Result(const Error& error) noexcept(std::is_nothrow_constructible_v<Error>) : storage_(std::in_place_type<Error>, error) {}

    /** @brief Returns whether the result contains a successful value. */
    [[nodiscard]] constexpr bool ok() const noexcept {
        return hasValue();
    }

    /** @brief Returns whether the result contains a successful value. */
    [[nodiscard]] constexpr bool hasValue() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    /**
     * @brief Returns a reference to the stored value.
     * @pre ok() is true.
     * @code{.cpp}
     * if (result) {
     *     do_something(result.value());
     * }
     * @endcode
     */
    [[nodiscard]] constexpr const T& value() const & {
        STRUO_ASSERT(ok(), "failed to check: !struo::Result");
        return std::get<0>(storage_);
    }

    /**
     * @brief Returns a reference to the stored value.
     * @pre ok() is true.
     * @code{.cpp}
     * if (result) {
     *     do_something(result.value());
     * }
     * @endcode
     */
    [[nodiscard]] constexpr T& value() & {
        STRUO_ASSERT(ok(), "failed to check: !struo::Result");
        return std::get<0>(storage_);
    }

    /**
     * @brief Returns a reference to the stored value.
     * @pre ok() is true.
     * @code{.cpp}
     * if (result) {
     *     do_something(result.value());
     * }
     * @endcode
     */
    [[nodiscard]] constexpr T&& value() && {
        STRUO_ASSERT(ok(), "failed to check: !struo::Result");
        return std::get<0>(std::move(storage_));
    }

    /**
     * @brief Returns a reference to the stored error.
     * @pre ok() is false.
     * @code{.cpp}
     * if (!result) {
     *     handle_error(result.error());
     * }
     * @endcode
     */
    [[nodiscard]] constexpr const Error& error() const & {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return std::get<1>(storage_);
    }

    /**
     * @brief Returns a reference to the stored error.
     * @pre ok() is false.
     * @code{.cpp}
     * if (!result) {
     *     handle_error(result.error());
     * }
     * @endcode
     */
    [[nodiscard]] constexpr Error& error() & {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return std::get<1>(storage_);
    }

    /**
     * @brief Returns a reference to the stored error.
     * @pre ok() is false.
     * @code{.cpp}
     * if (!result) {
     *     handle_error(result.error());
     * }
     * @endcode
     */
    [[nodiscard]] constexpr Error&& error() && {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return std::get<1>(std::move(storage_));
    }

    /** @brief Returns whether the result contains a successful value. */
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return hasValue();
    }

    /**
     * @brief Returns a reference to the stored value.
     * @pre ok() is true.
     * @code{.cpp}
     * if (result) {
     *     do_something(*result);
     * }
     * @endcode
     */
    [[nodiscard]] constexpr T&& operator*() && {
        return std::move(*this).value();
    }

    /**
     * @brief Returns a reference to the stored value.
     * @pre ok() is true.
     * @code{.cpp}
     * if (result) {
     *     do_something(*result);
     * }
     * @endcode
     */
    [[nodiscard]] constexpr T& operator*() & {
        return value();
    }

    /**
     * @brief Returns a reference to the stored value.
     * @pre ok() is true.
     * @code{.cpp}
     * if (result) {
     *     do_something(*result);
     * }
     * @endcode
     */
    [[nodiscard]] constexpr const T& operator*() const & {
        return value();
    }

private:
    std::variant<T, Error> storage_{};
};

/**
 * @brief Represents success without a value, or failure with an Error.
 *
 * Default construction represents success. Check ok() or the explicit boolean
 * conversion before accessing an error.
 * @code{.cpp}
 * struo::Result<void> result = validatePort(8080);
 * if (!result) {
 *     handle_error(result.error());
 * }
 * @endcode
 */
template<>
class [[nodiscard]] Result<void> {
public:
    /** @brief Constructs a successful result. */
    constexpr Result() = default;

    constexpr ~Result() = default;

    constexpr Result(Result&&) = default;
    constexpr Result(const Result&) = default;

    constexpr Result& operator=(Result&&) = default;
    constexpr Result& operator=(const Result&) = default;

    constexpr Result(Error&& error) noexcept(std::is_nothrow_move_constructible_v<Error>) : error_(std::in_place, std::move(error)) {}
    constexpr Result(const Error& error) noexcept(std::is_nothrow_copy_constructible_v<Error>) : error_(std::in_place, error) {}

    /** @brief Returns whether the result represents success. */
    [[nodiscard]] constexpr bool ok() const noexcept {
        return !error_.has_value();
    }

    /** @brief Returns whether the result represents success. */
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return ok();
    }

    /**
     * @brief Returns a reference to the stored error.
     * @pre ok() is false.
     * @code{.cpp}
     * if (!result) {
     *     handle_error(result.error());
     * }
     * @endcode
     */
    [[nodiscard]] constexpr const Error& error() const & {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return *error_;
    }

    /**
     * @brief Returns a reference to the stored error.
     * @pre ok() is false.
     * @code{.cpp}
     * if (!result) {
     *     handle_error(result.error());
     * }
     * @endcode
     */
    [[nodiscard]] constexpr Error& error() & {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return *error_;
    }

    /**
     * @brief Returns a reference to the stored error.
     * @pre ok() is false.
     * @code{.cpp}
     * if (!result) {
     *     handle_error(result.error());
     * }
     * @endcode
     */
    [[nodiscard]] constexpr Error&& error() && {
        STRUO_ASSERT(!ok(), "failed to check: struo::Result");
        return std::move(*error_);
    }

private:
    std::optional<Error> error_{};
};

/**
 * @brief Moves an error out of a failed result.
 * @tparam T The result's value type, including void.
 * @param result The failed result whose error is moved into the return value.
 * @return The extracted error.
 * @pre result.ok() is false.
 * @note The result remains failed and contains a moved-from error.
 *
 * @code{.cpp}
 * struo::Result<int> readPort();
 *
 * struo::Result<int> readCheckedPort() {
 *     auto port = readPort();
 *     if (!port) {
 *         return struo::err(port);
 *     }
 *     return *port;
 * }
 * @endcode
 */
template<typename T>
[[nodiscard]] constexpr Error err(Result<T>& result) {
    return std::move(result).error();
}

/**
 * @brief Constructs an Error with a category, message, and source location.
 * @param ec The error category.
 * @param message The diagnostic message.
 * @param where The source location; defaults to the caller's location.
 * @return An Error that can be returned as a failed Result<T> or Result<void>.
 *
 * @code{.cpp}
 * struo::Result<int> checkedPort(int port) {
 *     if (port < 1 || port > 65535) {
 *         return struo::err(struo::INVALID_VALUE, "Port must be between 1 and 65535");
 *     }
 *     return port;
 * }
 * @endcode
 */
[[nodiscard]] constexpr auto err(ErrorCode ec, std::string message, std::source_location where = std::source_location::current()) {
    return Error{ec, std::move(message), std::move(where)};
}

/**
 * @brief Constructs a successful result for an operation without a return value.
 * @return A successful Result<void>.
 *
 * @code{.cpp}
 * struo::Result<void> validatePort(int port) {
 *     if (port < 1 || port > 65535) {
 *         return struo::err(struo::INVALID_VALUE, "Port must be between 1 and 65535");
 *     }
 *     return struo::ok();
 * }
 * @endcode
 */
[[nodiscard]] constexpr auto ok() {
    return Result<void>{};
}

}
