#pragma once

#include <cstddef>

#include "struo/detail/types.hpp"

namespace struo {

/**
 * @brief Applies constraints or transforms to an optional's value when it is present.
 *
 * An empty optional succeeds without invoking any operations and remains empty.
 * Operations run in order, stopping at the first failure.
 *
 * @code{.cpp}
 * struct Config {
 *  std::optional<std::string> name;
 *  // ....
 * };
 *
 * Field<&Config::name>{
 *     Keys{"name"},
 *     Constraints{IfPresent<NotEmpty>},
 *     Transforms{IfPresent<TrimWhitespace, ToLower>}
 * }
 * @endcode
 */
template<auto... Inners>
inline constexpr detail::IfPresentT<Inners...> IfPresent{};

/**
 * @brief Applies constraints or transforms to every element of a sequence.
 *
 * Use in Constraints to check each element, or in Transforms to transform each
 * element. Operations run in order for each element, stopping at the first
 * failure. An empty sequence succeeds.
 *
 * @code{.cpp}
 * struct Config {
 *  std::vector<std::string> names;
 *  // ....
 * };
 *
 * Field<&Config::names>{
 *     Keys{"names"},
 *     Constraints{ForEach<NotEmpty>},
 *     Transforms{ForEach<TrimWhitespace, ToLower>}
 * }
 * @endcode
 */
template<auto... Inners>
inline constexpr detail::ForEachT<Inners...> ForEach{};

/**
 * @brief A string literal that can be used as a template argument.
 *
 * Stores the literal including its terminating null character. String literals
 * can be passed directly to templates taking a Str argument.
 *
 * @code{.cpp}
 * Field<&Config::name>{Keys{"name"}, Transforms{AddPrefix<"app-">}}
 * // AddPrefix<Str{"app-"}> is equivalent.
 * @endcode
 */
template <size_t N>
struct Str {
    char string[N];

    constexpr Str(const char (&str)[N]) {
        for (std::size_t i { 0 }; i < N; ++i)
            string[i] = str[i];
    }
};

} // namespace struo
