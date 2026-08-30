#pragma once

#include <tuple>
#include <concepts>

namespace struo::detail {

    template<typename T, typename>
    struct StrongAlias {
        T value;

        template<typename... Args>
        requires std::constructible_from<T, Args...>
        constexpr explicit StrongAlias(Args&&... args) : value{std::forward<Args>(args)...} {}
    };

    template<typename, typename... Args>
    struct ArgPack {
        std::tuple<Args...> values;

        constexpr explicit ArgPack(Args... args) : values{std::move(args)...} {}
    };

    template<typename Container, typename Tag, typename... Args>
    constexpr void apply_arg_pack(ArgPack<Tag, Args...>&& pack, Container& container) {
        std::apply([&](auto&&... values) {
            (container.emplace_back(std::move(values)), ...);
        }, std::move(pack.values));
    }
}