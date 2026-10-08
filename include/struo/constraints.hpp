#pragma once

#include "struo/types.hpp"
#include <cstddef>
#include <limits>

#include "struo/detail/constraints/range.hpp"
#include "struo/detail/constraints/combinators.hpp"
#include "struo/detail/constraints/platform.hpp"
#include "struo/detail/constraints/value.hpp"

namespace struo {

/**
 * @brief Requires a value between Min and Max, inclusive.
 *
 * Bounds must have the same type, with Min <= Max. Use bounds matching
 * the field's value type.
 *
 * @code{.cpp}
 * Field<&Config::count>{Keys{"count"}, Constraints{Range<1, 3>}}
 * @endcode
 */
template<auto Min, auto Max>
requires IsValidRangeBounds<Min, Max>
inline constexpr auto Range { detail::RangeConstraint<Min, Max>{} };

/**
 * @brief Requires a value greater than or equal to Min.
 *
 * Use a bound matching the field's numeric type.
 *
 * @code{.cpp}
 * Field<&Config::count>{Keys{"count"}, Constraints{AtLeast<1>}}
 * @endcode
 */
template<auto Min>
requires (std::numeric_limits<decltype(Min)>::is_specialized && IsValidRangeBounds<Min, std::numeric_limits<decltype(Min)>::max()>)
inline constexpr auto AtLeast { detail::RangeConstraint<Min, std::numeric_limits<decltype(Min)>::max()>{} };

/**
 * @brief Requires a value less than or equal to Max.
 *
 * Use a bound matching the field's numeric type.
 *
 * @code{.cpp}
 * Field<&Config::count>{Keys{"count"}, Constraints{AtMost<10>}}
 * @endcode
 */
template<auto Max>
requires (std::numeric_limits<decltype(Max)>::is_specialized && IsValidRangeBounds<std::numeric_limits<decltype(Max)>::lowest(), Max>)
inline constexpr auto AtMost { detail::RangeConstraint<std::numeric_limits<decltype(Max)>::lowest(), Max>{} };

/**
 * @brief Requires a string or container size between Min and Max, inclusive.
 *
 * @code{.cpp}
 * Field<&Config::name>{Keys{"name"}, Constraints{SizeRange<1, 32>}}
 * @endcode
 */
template<size_t Min, size_t Max>
requires (Min <= Max)
inline constexpr auto SizeRange { detail::SizeRangeConstraint<Min, Max>{} };

/**
 * @brief Requires a string or container to contain at least Min elements.
 *
 * @code{.cpp}
 * Field<&Config::servers>{Keys{"servers"}, Constraints{SizeAtLeast<1>}}
 * @endcode
 */
template<size_t Min>
inline constexpr auto SizeAtLeast { detail::SizeRangeConstraint<Min, std::numeric_limits<size_t>::max()>{} };

/**
 * @brief Requires a string or container to contain at most Max elements.
 *
 * @code{.cpp}
 * Field<&Config::servers>{Keys{"servers"}, Constraints{SizeAtMost<10>}}
 * @endcode
 */
template<size_t Max>
inline constexpr auto SizeAtMost { detail::SizeRangeConstraint<std::numeric_limits<size_t>::min(), Max>{} };

/**
 * @brief Requires a string or container to contain exactly N elements.
 *
 * @code{.cpp}
 * Field<&Config::coordinates>{Keys{"coordinates"}, Constraints{SizeExactly<3>}}
 * @endcode
 */
template<size_t N>
inline constexpr auto SizeExactly { detail::SizeRangeConstraint<N, N>{} };

/**
 * @brief Requires at least one of the supplied constraints to succeed.
 *
 * Stops checking after the first success.
 *
 * @code{.cpp}
 * Field<&Config::count>{Keys{"count"}, Constraints{Or<Range<1, 3>, Range<7, 9>>}}
 * @endcode
 */
template<auto... Constraints>
inline constexpr auto Or { detail::OrConstraint<Constraints...>{} };

/**
 * @brief Requires every supplied constraint to succeed.
 *
 * Stops checking after the first failure.
 *
 * @code{.cpp}
 * Field<&Config::count>{Keys{"count"}, Constraints{And<Positive, AtMost<10>>}}
 * @endcode
 */
template<auto... Constraints>
inline constexpr auto And { detail::AndConstraint<Constraints...>{} };

/**
 * @brief Requires the supplied constraint to fail.
 *
 * @code{.cpp}
 * Field<&Config::count>{Keys{"count"}, Constraints{Not<OneOf<0, 1>>}}
 * @endcode
 */
template<auto Constraint>
inline constexpr auto Not { detail::NotConstraint<Constraint>{} };

/**
 * @brief Requires exactly one of the supplied constraints to succeed.
 *
 * A value matching none or more than one fails.
 *
 * @code{.cpp}
 * // Accepts 1–9 except 5, which matches both ranges.
 * Field<&Config::count>{Keys{"count"}, Constraints{ExactlyOne<Range<1, 5>, Range<5, 9>>}}
 * @endcode
 */
template<auto... Constraints>
inline constexpr auto ExactlyOne { detail::ExactlyOneConstraint<Constraints...>{} };

/**
 * @brief Requires a value equal to one of the supplied values.
 *
 * @code{.cpp}
 * Field<&Config::port>{Keys{"port"}, Constraints{OneOf<80, 443>}}
 * @endcode
 */
template<auto... Values>
inline constexpr auto OneOf { detail::OneOfConstraint<Values...>{} };

/**
 * @brief Requires a value strictly greater than zero.
 *
 * @code{.cpp}
 * Field<&Config::count>{Keys{"count"}, Constraints{Positive}}
 * @endcode
 */
inline constexpr auto Positive { detail::PositiveConstraint{} };

/**
 * @brief Requires a string or container to be nonempty.
 *
 * @code{.cpp}
 * Field<&Config::name>{Keys{"name"}, Constraints{NotEmpty}}
 * @endcode
 */
inline constexpr auto NotEmpty { detail::NotEmptyConstraint{} };

/**
 * @brief Requires a string to start with Prefix.
 *
 * The comparison is case-sensitive.
 *
 * @code{.cpp}
 * Field<&Config::url>{Keys{"url"}, Constraints{StartsWith<"https://">}}
 * @endcode
 */
template<Str Prefix>
inline constexpr detail::StartsWithConstraint<Prefix> StartsWith{};

/**
 * @brief Requires a string to end with Postfix.
 *
 * The comparison is case-sensitive.
 *
 * @code{.cpp}
 * Field<&Config::filename>{Keys{"filename"}, Constraints{EndsWith<".json">}}
 * @endcode
 */
template<Str Postfix>
inline constexpr detail::EndsWithConstraint<Postfix> EndsWith{};

/**
 * @brief Rejects infinity and NaN for floating-point values.
 *
 * Values of other types always pass.
 *
 * @code{.cpp}
 * Field<&Config::threshold>{Keys{"threshold"}, Constraints{IsFinite}}
 * @endcode
 */
inline constexpr detail::IsFiniteConstraint IsFinite{};

/**
 * @brief Requires a path to refer to an existing filesystem entry.
 *
 * Existing directories also pass. Relative paths are resolved against the
 * current working directory.
 *
 * @code{.cpp}
 * Field<&Config::input>{Keys{"input"}, Constraints{FileExists}}
 * @endcode
 */
inline constexpr auto FileExists { detail::FileExistsConstraint{} };

/**
 * @brief Requires a path to refer to an existing directory.
 *
 * Relative paths are resolved against the current working directory.
 *
 * @code{.cpp}
 * Field<&Config::output>{Keys{"output"}, Constraints{DirectoryExists}}
 * @endcode
 */
inline constexpr auto DirectoryExists { detail::DirectoryExistsConstraint{} };

/**
 * @brief Requires a port number between 0 and 65535, inclusive.
 *
 * Zero is accepted to support automatic port allocation when binding a socket.
 * Use Range<1, 65535> when a specific nonzero port is required.
 * The field's integer type must be able to represent both bounds.
 *
 * @code{.cpp}
 * Field<&Config::port>{Keys{"port"}, Constraints{IsValidPort}}
 * @endcode
 */
inline constexpr auto IsValidPort { Range<0, 65535> };

#if STRUO_PLATFORM_LINUX

/**
 * @brief Requires a string with valid hostname syntax.
 *
 * Does not check DNS resolution or reachability.
 *
 * @code{.cpp}
 * Field<&Config::hostname>{Keys{"hostname"}, Constraints{IsValidHostname}}
 * @endcode
 */
inline constexpr auto IsValidHostname { detail::IsValidHostnameConstraint{} };

/**
 * @brief Requires a string containing a valid IPv6 address.
 *
 * Accepts compressed addresses such as "::1".
 * Hostnames and CIDR prefixes are not accepted.
 *
 * @code{.cpp}
 * Field<&Config::address>{Keys{"address"}, Constraints{IsValidIpv6}}
 * @endcode
 */
inline constexpr auto IsValidIpv6 { detail::IsValidIpv6Constraint{} };

/**
 * @brief Requires a string containing a valid dotted-decimal IPv4 address.
 *
 * Hostnames and CIDR prefixes are not accepted.
 *
 * @code{.cpp}
 * Field<&Config::address>{Keys{"address"}, Constraints{IsValidIpv4}}
 * @endcode
 */
inline constexpr auto IsValidIpv4 { detail::IsValidIpv4Constraint{} };

#endif // STRUO_PLATFORM_LINUX

} // namespace struo
