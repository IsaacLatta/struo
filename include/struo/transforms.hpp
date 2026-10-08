#pragma once

#include "struo/types.hpp"
#include "struo/detail/transforms.hpp"

namespace struo {

template<char Char>
inline constexpr detail::TrimT<Char> Trim{};

inline constexpr auto TrimWhitespace { Trim<' '> };

template<Str Prefix>
inline constexpr detail::AddPrefixT<Prefix> AddPrefix{};

inline constexpr auto AddLeadingSlash { AddPrefix<Str{"/"}> };

inline constexpr detail::ToUpperT ToUpper{};

inline constexpr detail::ToLowerT ToLower{};

template<Str ParentDir>
inline constexpr detail::RelativeToT<ParentDir> RelativeTo{};

} // namespace struo
