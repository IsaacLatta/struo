#pragma once

#include "struo/types.hpp"
#include "struo/detail/defaults.hpp"

namespace struo {

/**
 * @brief Uses V as the default value when a field is missing.
 *
 * @code{.cpp}
 * Field<&Config::port>{Keys{"port"}, Defaults{Value<8080>}}
 * @endcode
 */
template<auto V>
inline constexpr detail::ValueT<V> Value{};

/**
 * @brief Reads a numeric default from the environment variable named Key.
 *
 * Used when the field is missing. Loading fails if the variable is unset
 * or its value cannot be converted to the field's numeric type.
 *
 * @code{.cpp}
 * Field<&Config::port>{Keys{"port"}, Defaults{FromEnv<"APP_PORT">}}
 * @endcode
 */
template<Str Key>
inline constexpr detail::FromEnvT<Key> FromEnv{};

} // namespace struo
