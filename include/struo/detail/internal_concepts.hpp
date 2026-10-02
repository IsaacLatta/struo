#pragma once

#include <concepts>
#include <type_traits>
#include <string_view>
#include <utility>

namespace struo::detail {

template<typename T, typename... Args>
concept IsBraceConstructableFrom = requires(Args&&... args) {
    T{std::forward<Args>(args)...};
};

template<typename T>
concept IsStringLike = std::convertible_to<const T&, std::string_view>;

template<typename T>
concept HasName = requires {
    { T::name() } -> std::convertible_to<std::string_view>;
};

template<typename T>
concept HasDescription = requires {
    { T::description() } -> std::convertible_to<std::string_view>;
};

}