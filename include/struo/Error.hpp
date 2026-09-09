#pragma once

#include <source_location>
#include <string>
#include <string_view>
#include <utility>

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

}
