#pragma once

#include <cstdlib>
#include <format>
#include <print>
#include <source_location>
#include <stacktrace>
#include <string_view>
#include <utility>

namespace struo::detail {

    [[nodiscard]] constexpr std::string_view file_basename(std::string_view path) noexcept {
        const auto pos = path.find_last_of("/\\");
        return pos == std::string_view::npos ? path : path.substr(pos + 1);
    }

    [[noreturn]]
    inline void on_assertion(const std::stacktrace& trace, const std::source_location& where, std::string_view condition) noexcept {
        try {
            std::println(stderr, "STRUO CHECK FAILED: check {} ", condition);
            std::println(stderr, "at {}:{} in {}\n{}", file_basename(where.file_name()), where.line(), where.function_name(), trace);
        }
        catch (...) {}
        std::abort();
    }

    template<typename... Args>
    [[noreturn]] void on_assertion(
        const std::stacktrace& trace,
        const std::source_location& where,
        std::string_view condition,
        std::format_string<Args...> fmt,
        Args&&... args) noexcept {
        try {
            std::print(stderr, "STRUO CHECK FAILED: check {} | ", condition);
            std::print(stderr, fmt, std::forward<Args>(args)...);
            std::println(stderr, " at {}:{} in {}\n{}", file_basename(where.file_name()), where.line(), where.function_name(), trace);
        }
        catch (...) {}
        std::abort();
    }
}

#define STRUO_CHECK_IMPL(cond, ...) do { \
    if(!(cond)) [[unlikely]] { \
        struo::detail::on_assertion( \
            std::stacktrace::current(), \
            std::source_location::current(), \
            #cond __VA_OPT__(,) __VA_ARGS__); \
    } \
} while (false)
