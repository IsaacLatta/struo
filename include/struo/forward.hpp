#pragma once

#include <optional>
#include <string_view>
#include <vector>
#include <functional>

#include "detail/StrongAlias.hpp"

namespace struo {

    class Error;

    template<typename T>
    requires (!std::is_reference_v<T> && !std::same_as<std::remove_cvref_t<T>, Error>)
    class Result;

    class YamlParser;

    struct DefaultSchema {};

    template<typename T, typename Schema = DefaultSchema>
    struct Traits;

    using Keys = detail::TaggedAlias<std::vector<std::string_view>, struct TagKeys>;

    using Description = detail::TaggedAlias<std::string_view, struct TagDescription>;

    template<typename... Callables>
    using Defaults = detail::TaggedArgPack<struct TagDefaults, Callables...>;

    template<typename... Callables>
    using Constraints = detail::TaggedArgPack<struct TagConstraints, Callables...>;
}
