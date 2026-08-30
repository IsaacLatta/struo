#pragma once

#include <tuple>

namespace struo::detail {

    template<typename T, typename>
    struct StrongAlias {
        T value;

        template<typename... Args>
        requires std::constructible_from<T, Args...>
        constexpr explicit StrongAlias(Args&&... args) : value{std::forward<Args>(args)...} {}
    };

    template<typename, typename... Args>
    struct ArgList {
        std::tuple<Args...> values;

        constexpr explicit ArgList(Args&&... args) : values{std::forward<args>(args)...} {}
    };

}