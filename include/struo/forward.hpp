#pragma once

#include <optional>
#include <string_view>
#include <vector>
#include <functional>

#include "detail/StrongAlias.hpp"

namespace struo {

    class Error;

    struct DefaultSchema {};

    template<typename T, typename Schema = DefaultSchema>
    struct Traits;

    using Key = detail::StrongAlias<std::string_view, struct TagKey>;

    using Alias = detail::StrongAlias<std::string_view, struct TagAlias>;
    using Aliases = std::vector<Alias>;

    using Description = detail::StrongAlias<std::string_view, struct TagDescription>;

    template<typename T>
    using ObjectDefault = detail::StrongAlias<std::function<typename T::value_type(const T&)>, struct TagObjectDefault>;

    template<typename... Ts>
    using ObjectDefaults = detail::ArgList<struct TagObjectDefaults, Ts...>;

    template<typename... Ts>
    using ValueDefaults = detail::ArgList<struct TagValueDefaults, Ts...>;

    template<typename... Ts>
    using ObjectConstraint = detail::StrongAlias<std::optional<Error>, struct TagObjectConstraint>;

    template<typename... Ts>
    using ObjectConstraints = detail::ArgList<struct TagObjectConstraints, Ts...>;

    template<typename... Ts>
    using ValueConstraints = detail::ArgList<struct TagValueConstraints, Ts...>;
}