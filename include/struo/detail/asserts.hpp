#pragma once

#if defined(STRUO_NO_ASSERT)
    #define STRUO_ASSERT(cond, ...) ((void)0)
#else // STRUO_NO_ASSERT

#include <cstdio>
#include <cstdlib>
#include <format>
#include <source_location>
#include <string_view>
#include <utility>

namespace struo::detail {

    template<typename... Args>
    [[noreturn]] void print_to_stderr_and_abort(const std::source_location& where, std::string_view condition, std::format_string<Args...> fmt, Args&&... args) noexcept {
        try {
            static constexpr auto get_file_name = [](std::string_view path) {
                const auto pos = path.find_last_of("/\\");
                return pos == std::string_view::npos ? path : path.substr(pos + 1);
            };

            const auto message = std::format(
                "STRUO CHECK FAILED: check {} | {} at {}:{} in {}\n",
                condition,
                std::format(fmt, std::forward<Args>(args)...),
                get_file_name(where.file_name()),
                where.line(),
                where.function_name()
            );

            std::fputs(message.c_str(), stderr);
        } catch (...) {}

        std::abort();
    }

    [[noreturn]] inline void print_to_stderr_and_abort(const std::source_location& where, std::string_view condition) noexcept {
        print_to_stderr_and_abort(where, condition, "");
    }
}

#define STRUO_ASSERT(cond, ...) do { \
    if(!(cond)) [[unlikely]] { \
        ::struo::detail::print_to_stderr_and_abort( \
            std::source_location::current(), \
            #cond __VA_OPT__(,) __VA_ARGS__); \
    } \
} while (false)

#endif // !STRUO_NO_ASSERT
