#pragma once

#include <string>
#include <source_location>
#include <expected>

#include "struo/forward.hpp"

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
        SYNTAX_ERROR
    }; using enum ErrorCode;

    class Error {
    public:
        constexpr explicit Error(ErrorCode ec, std::string message, std::source_location where = std::source_location::current())
            : code_(ec), message_(std::move(message)), location_(where) {}

        [[nodiscard]] constexpr std::string_view what() const noexcept {
            return message_;
        }

        [[nodiscard]] constexpr std::source_location where() const noexcept {
            return location_;
        }

    private:
        ErrorCode code_{}; // maybe std::error_code instead, user can plug in their errors? Then we duplicate "message"?
        std::string message_{};
        std::source_location location_{};
    };

    [[nodiscard]] constexpr auto err(ErrorCode ec, std::string message, std::source_location where = std::source_location::current()) {
        return std::unexpected{Error{ec, std::move(message), where}};
    }

}