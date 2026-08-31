#pragma once

#include <optional>
#include <string_view>
#include <vector>
#include <functional>

#include "detail/StrongAlias.hpp"

namespace struo {

    enum class FileFormat {
        YAML
    };

    class Error;

    struct DefaultSchema {};

    template<typename T, typename Schema = DefaultSchema>
    struct Traits;

    using Key = detail::StrongAlias<std::string_view, struct TagKey>;

    using Alias = detail::StrongAlias<std::string_view, struct TagAlias>;
    using Aliases = std::vector<Alias>;

    using Description = detail::StrongAlias<std::string_view, struct TagDescription>;

    template<typename Object>
    using ObjectDefault = detail::StrongAlias<std::function<std::optional<typename Object::value_type>(const Object&)>, struct TagObjectDefault>;

    template<typename... Ts>
    using ObjectDefaults = detail::ArgPack<struct TagObjectDefaults, Ts...>;

    template<typename T>
    using ValueDefault = detail::StrongAlias<std::function<std::optional<T>()>, struct TagValueDefault>;

    template<typename... Ts>
    using ValueDefaults = detail::ArgPack<struct TagValueDefaults, Ts...>;

    template<typename Object, typename... Ts>
    using ObjectConstraint = detail::StrongAlias<std::function<std::optional<Error>(const Object&)>, struct TagObjectConstraint>;

    template<typename... Ts>
    using ObjectConstraints = detail::ArgPack<struct TagObjectConstraints, Ts...>;

    template<typename T>
    using ValueConstraint = detail::StrongAlias<std::function<std::optional<Error>(const T&)>, struct TagObjectConstraint>;

    template<typename... Ts>
    using ValueConstraints = detail::ArgPack<struct TagValueConstraints, Ts...>;
}