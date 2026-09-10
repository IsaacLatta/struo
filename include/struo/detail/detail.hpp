#pragma once

#include <string>
#include <string_view>
#include <charconv>
#include <concepts>
#include <optional>

#include <magic_enum/magic_enum.hpp>

namespace struo::detail {

    template <typename T>
    std::optional<T> from_string(std::string_view str) {
        T value{};
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
        if (ec != std::errc{} || ptr != str.data() + str.size()) {
            return std::nullopt;
        }
        return value;
    }

    template<typename T>
    requires std::is_enum_v<T>
    [[nodiscard]] constexpr std::string_view enum_name(T t) {
        return magic_enum::enum_name(t);
    }
}
