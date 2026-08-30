#pragma once

#include <string_view>

namespace struo {

[[nodiscard]] constexpr std::string_view get_project_version() {
    return STRUO_VERSION;
}

[[nodiscard]] constexpr bool is_debug_enabled() {
    return static_cast<bool>(STRUO_DEBUG);
}

}