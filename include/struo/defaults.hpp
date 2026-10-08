#pragma once

#include "struo/types.hpp"
#include "struo/detail/defaults.hpp"

namespace struo {

template<auto V>
inline constexpr detail::ValueT<V> Value{};

template<Str Key>
inline constexpr detail::FromEnvT<Key> FromEnv{};

} // namespace struo
