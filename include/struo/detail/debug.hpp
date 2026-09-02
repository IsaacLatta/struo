#pragma once

#include <cstdio>
#include <cstdlib>
#include <format>
#include <source_location>
#include <string_view>
#include <utility>

namespace struo::detail {

    [[nodiscard]] constexpr std::string_view file_basename(std::string_view path) noexcept {
        const auto pos = path.find_last_of("/\\");
        return pos == std::string_view::npos ? path : path.substr(pos + 1);
    }

    template<typename... Args>
    [[noreturn]] void on_assertion(const std::source_location& where, std::string_view condition, std::format_string<Args...> fmt, Args&&... args) noexcept {
        try {
            const auto message = std::format(
                "STRUO CHECK FAILED: check {} | {} at {}:{} in {}\n",
                condition,
                std::format(fmt, std::forward<Args>(args)...),
                file_basename(where.file_name()),
                where.line(),
                where.function_name()
            );

            std::fputs(message.c_str(), stderr);
        } catch (...) {}

        std::abort();
    }

    [[noreturn]] inline void on_assertion(const std::source_location& where, std::string_view condition) noexcept {
        on_assertion(where, condition, "");
    }

}

#define STRUO_CHECK_IMPL(cond, ...) do { \
    if(!(cond)) [[unlikely]] { \
        struo::detail::on_assertion( \
            std::source_location::current(), \
            #cond __VA_OPT__(,) __VA_ARGS__); \
    } \
} while (false)
