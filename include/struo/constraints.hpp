#pragma once

#include "struo/types.hpp"
#include <cstddef>
#include <limits>

#include "struo/detail/constraints/range.hpp"
#include "struo/detail/constraints/combinators.hpp"
#include "struo/detail/constraints/platform.hpp"
#include "struo/detail/constraints/value.hpp"

namespace struo {

template<auto Min, auto Max>
requires IsValidRangeBounds<Min, Max>
inline constexpr auto Range { detail::RangeConstraint<Min, Max>{} };

template<auto Min>
requires (std::numeric_limits<decltype(Min)>::is_specialized &&
    IsValidRangeBounds<Min, std::numeric_limits<decltype(Min)>::max()>)
inline constexpr auto AtLeast { detail::RangeConstraint<Min, std::numeric_limits<decltype(Min)>::max()>{} };

template<auto Max>
requires (std::numeric_limits<decltype(Max)>::is_specialized &&
    IsValidRangeBounds<std::numeric_limits<decltype(Max)>::lowest(), Max>)
inline constexpr auto AtMost { detail::RangeConstraint<std::numeric_limits<decltype(Max)>::lowest(), Max>{} };

template<size_t Min, size_t Max>
requires (Min <= Max)
inline constexpr auto SizeRange { detail::SizeRangeConstraint<Min, Max>{} };

template<size_t Min>
inline constexpr auto SizeAtLeast { detail::SizeRangeConstraint<Min, std::numeric_limits<size_t>::max()>{} };

template<size_t Max>
inline constexpr auto SizeAtMost { detail::SizeRangeConstraint<std::numeric_limits<size_t>::min(), Max>{} };

template<size_t N>
inline constexpr auto SizeExactly { detail::SizeRangeConstraint<N, N>{} };

template<auto... Constraints>
inline constexpr auto Or { detail::OrConstraint<Constraints...>{} };

template<auto... Constraints>
inline constexpr auto And { detail::AndConstraint<Constraints...>{} };

template<auto Constraint>
inline constexpr auto Not { detail::NotConstraint<Constraint>{} };

template<auto... Constraints>
inline constexpr auto ExactlyOne { detail::ExactlyOneConstraint<Constraints...>{} };

template<auto... Values>
inline constexpr auto OneOf { detail::OneOfConstraint<Values...>{} };

inline constexpr auto Positive { detail::PositiveConstraint{} };

inline constexpr auto NotEmpty { detail::NotEmptyConstraint{} };

template<Str Prefix>
inline constexpr detail::StartsWithConstraint<Prefix> StartsWith{};

template<Str Postfix>
inline constexpr detail::EndsWithConstraint<Postfix> EndsWith{};

inline constexpr detail::IsFiniteConstraint IsFinite{};

inline constexpr auto FileExists { detail::FileExistsConstraint{} };

inline constexpr auto DirectoryExists { detail::DirectoryExistsConstraint{} };

inline constexpr auto IsValidPort { Range<0, 65535> };

#if STRUO_PLATFORM_LINUX

inline constexpr auto IsValidHostname { detail::IsValidHostnameConstraint{} };

inline constexpr auto IsValidIpv6 { detail::IsValidIpv6Constraint{} };

inline constexpr auto IsValidIpv4 { detail::IsValidIpv4Constraint{} };

#endif // STRUO_PLATFORM_LINUX

} // namespace struo
