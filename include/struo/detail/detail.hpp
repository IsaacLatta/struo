#pragma once

#include <string>
#include <string_view>
#include <charconv>
#include <concepts>
#include <optional>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

namespace struo::detail {

    template<typename T>
    concept HasName = requires {
        { T::name() } -> std::convertible_to<std::string_view>;
    };

    template<typename T>
    concept HasDescription = requires {
        { T::description() } -> std::convertible_to<std::string_view>;
    };

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


    template<typename T>
    requires std::is_enum_v<T> || IsStringLike<T>
    [[nodiscard]] constexpr std::string_view as_string(T value) {
        if constexpr (std::is_enum_v<T>) {
            return detail::enum_name(value);
        } else {
            return value;
        }
    }

    template<typename T>
    [[nodiscard]] constexpr std::string_view name_of() {
        using U = std::remove_cvref_t<T>;
        if constexpr (HasName<U>) {
            return U::name();
        } else {
            return "<unnamed>";
        }
    }

    template<typename T>
    [[nodiscard]] constexpr std::string description_of() {
        using U = std::remove_cvref_t<T>;
        if constexpr (HasDescription<U>) {
            return std::string{U::description()};
        } else {
            return {};
        }
    }
}
