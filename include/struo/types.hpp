#pragma once

#include <cstddef>

#include "struo/detail/types.hpp"

namespace struo {

template<auto... Inners>
inline constexpr detail::IfPresentT<Inners...> IfPresent{};

template<auto... Inners>
inline constexpr detail::ForEachT<Inners...> ForEach{};

template <std::size_t N>
struct Str {
    char string[N];

    constexpr Str(const char (&str)[N]) {
        for (std::size_t i { 0 }; i < N; ++i)
            string[i] = str[i];
    }
};

} // namespace struo
