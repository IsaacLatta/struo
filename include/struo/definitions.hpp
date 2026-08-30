#pragma once

#include <string_view>

#include "struo/detail/debug.hpp"

namespace struo {

[[nodiscard]] constexpr std::string_view get_project_version() noexcept {
    return STRUO_VERSION;
}

[[nodiscard]] constexpr bool is_debug_enabled() noexcept {
    return static_cast<bool>(STRUO_DEBUG);
}

}

#define STRUO_CHECK(cond, ...) \
    STRUO_CHECK_IMPL(cond __VA_OPT__(,) __VA_ARGS__)

#define STRUO_DCHECK(cond, ...) do {\
    if constexpr (struo::is_debug_enabled()) { \
        STRUO_CHECK_IMPL(cond __VA_OPT__(,) __VA_ARGS__); \
    } \
} while (false)

