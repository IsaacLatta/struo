#pragma once

#include <concepts>
#include <type_traits>

#include "struo/types.hpp"

namespace struo {
    class Error;

    template<typename T>
    requires (!std::is_reference_v<T> && !std::same_as<std::remove_cvref_t<T>, Error>)
    class Result;
}

namespace struo::detail {

class YamlParser;
class JsonParser;
class TomlParser;

template<auto... Constraints>
struct ForEachConstraint;

template<auto... Constraints>
struct IfPresentConstraintT;

} // namespace struo::detail
