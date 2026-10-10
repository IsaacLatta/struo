#pragma once

#include "struo/types.hpp"
#include "struo/detail/transforms.hpp"

namespace struo {

/**
 * @brief Removes occurrences of Char from both ends of a string or path.
 *
 * Characters inside the value are preserved.
 *
 * @code{.cpp}
 * Field<&Config::name>{Keys{"name"}, Transforms{Trim<' '>}}
 * @endcode
 */
template<char... Chars>
inline constexpr detail::TrimT<Chars...> Trim{};

/**
 * @brief Removes standard C whitespace from both ends of a string or path.
 *
 * Trims spaces, tabs, newlines, carriage returns, form feeds, and vertical tabs.
 * Whitespace inside the value is preserved.
 *
 * @code{.cpp}
 * Field<&Config::name>{Keys{"name"}, Transforms{TrimWhitespace}}
 * @endcode
 */
inline constexpr auto TrimWhitespace = Trim<' ', '\t', '\n', '\r', '\f', '\v'>;

/**
 * @brief Prepends Prefix to a string unless it already starts with Prefix.
 *
 * The prefix comparison is case-sensitive. Empty strings become Prefix.
 *
 * @code{.cpp}
 * Field<&Config::name>{Keys{"name"}, Transforms{AddPrefix<"app-">}}
 * @endcode
 */
template<Str Prefix>
inline constexpr detail::AddPrefixT<Prefix> AddPrefix{};

/**
 * @brief Adds a leading slash to a nonempty string unless it already has one.
 *
 * Empty strings remain empty.
 *
 * @code{.cpp}
 * Field<&Config::route>{Keys{"route"}, Transforms{AddLeadingSlash}}
 * @endcode
 */
inline constexpr auto AddLeadingSlash { AddPrefix<Str{"/"}> };

/**
 * @brief Converts a string or path to uppercase.
 *
 * Conversion follows the current C locale and does not perform Unicode case conversion.
 *
 * @code{.cpp}
 * Field<&Config::name>{Keys{"name"}, Transforms{ToUpper}}
 * @endcode
 */
inline constexpr detail::ToUpperT ToUpper{};

/**
 * @brief Converts a string or path to lowercase.
 *
 * Conversion follows the current C locale and does not perform Unicode case conversion.
 *
 * @code{.cpp}
 * Field<&Config::name>{Keys{"name"}, Transforms{ToLower}}
 * @endcode
 */
inline constexpr detail::ToLowerT ToLower{};

/**
 * @brief Resolves a relative string or path against ParentDir.
 *
 * Absolute paths remain unchanged. Joins paths without accessing the filesystem
 * or normalizing components such as "..". An empty value resolves to ParentDir.
 *
 * @code{.cpp}
 * Field<&Config::input>{Keys{"input"}, Transforms{RelativeTo<"/app/data">}}
 * @endcode
 */
template<Str ParentDir>
inline constexpr detail::RelativeToT<ParentDir> RelativeTo{};

} // namespace struo
