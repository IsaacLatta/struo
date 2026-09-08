#pragma once

#include <optional>
#include <string_view>
#include <vector>
#include <functional>

#include "detail/StrongAlias.hpp"

namespace struo {

    class Error;

    class YamlParser;

    struct DefaultSchema {};

    template<typename T, typename Schema = DefaultSchema>
    struct Traits;

    using Keys = detail::TaggedAlias<std::vector<std::string_view>, struct TagKeys>;

    using Description = detail::TaggedAlias<std::string_view, struct TagDescription>;

    template<typename Object>
    using ObjectDefault = detail::TaggedAlias<std::function<std::optional<typename Object::value_type>(const Object&)>, struct TagObjectDefault>;

    template<typename... Ts>
    using ObjectDefaults = detail::TaggedArgPack<struct TagObjectDefaults, Ts...>;

    template<typename T>
    using ValueDefault = detail::TaggedAlias<std::function<std::optional<T>()>, struct TagValueDefault>;

    template<typename... Ts>
    using ValueDefaults = detail::TaggedArgPack<struct TagValueDefaults, Ts...>;

    template<typename Object, typename... Ts>
    using ObjectConstraint = detail::TaggedAlias<std::function<std::optional<Error>(const Object&)>, struct TagObjectConstraint>;

    template<typename... Ts>
    using ObjectConstraints = detail::TaggedArgPack<struct TagObjectConstraints, Ts...>;

    template<typename T>
    using ValueConstraint = detail::TaggedAlias<std::function<std::optional<Error>(const T&)>, struct TagObjectConstraint>;

    template<typename... Ts>
    using ValueConstraints = detail::TaggedArgPack<struct TagValueConstraints, Ts...>;
}
