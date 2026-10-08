#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

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

/**
 * @brief Names the configuration keys accepted by a field.
 *
 * Keys are checked in order; the first present key is used. Empty Keys allow
 * a field to be supplied only by defaults. Key strings must remain alive while
 * the schema is used.
 *
 * @code{.cpp}
 * Field<&Config::port>{Keys{"port", "listen_port"}}
 * @endcode
 */
using Keys = detail::TaggedAlias<std::vector<std::string_view>, struct TagKeys>;

/**
 * @brief Supplies default values for a field when none of its keys is present.
 *
 * Each default is a callable invoked without arguments to supply a field value.
 * Defaults are tried in order until one supplies a value. Returning an empty
 * optional tries the next default; returning an error stops loading.
 *
 * @code{.cpp}
 * Field<&Config::port>{Keys{"port"}, Defaults{Value<8080>}}
 * @endcode
 */
template<typename... Callables>
using Defaults = detail::TaggedArgPack<struct TagDefaults, Callables...>;

/**
 * @brief Checks a field's resolved value after its transforms have run.
 *
 * Each constraint is a callable that accepts the field value and returns
 * Result<void>. Constraints run in order, stopping loading at the first failure.
 *
 * @code{.cpp}
 * Field<&Config::port>{Keys{"port"}, Constraints{Range<1, 65535>}}
 * @endcode
 */
template<typename... Callables>
using Constraints = detail::TaggedArgPack<struct TagConstraints, Callables...>;

/**
 * @brief Transforms a field's resolved value before its constraints are checked.
 *
 * Each transform is a callable that accepts the field value and returns a value
 * of the field's type or a Result containing it.
 * Transforms run in order, passing each result to the next transform. An error
 * stops loading.
 *
 * @code{.cpp}
 * Field<&Config::name>{Keys{"name"}, Transforms{TrimWhitespace, ToLower}}
 * @endcode
 */
template<typename... Callables>
using Transforms = detail::TaggedArgPack<struct TagTransforms, Callables...>;

/**
 * @brief Specifies whether a field must resolve to a value.
 *
 * REQUIRED fails loading if neither a key nor a default supplies a value.
 * OPTIONAL, retains the member initializer when no value is supplied.
 */
enum class Presence {
    REQUIRED,
    OPTIONAL
}; using enum Presence;

} // namespace struo
