#pragma once

#include <tuple>
#include <concepts>
#include <functional>

namespace struo::detail {

    template<typename T, typename... Args>
    concept IsBraceConstructableFrom = requires(Args&&... args) {
        T{std::forward<Args>(args)...};
    };

    template<typename T, typename Tag>
    struct TaggedAlias {
        T value;

        template<typename... Args>
        requires IsBraceConstructableFrom<T, Args...>
        constexpr explicit TaggedAlias(Args&&... args) : value{std::forward<Args>(args)...} {}
    };

    template<typename Tag, typename... Args>
    struct TaggedArgPack {
        std::tuple<Args...> values;

        constexpr explicit TaggedArgPack(Args... args) : values{std::move(args)...} {}
    };

    template<typename ReturnType, typename Container, typename Tag, typename... Callables>
    constexpr void apply_and_wrap_arg_func_pack(TaggedArgPack<Tag, Callables...> pack, Container& container) {
        std::apply([&](auto&&... callable){
            (container.emplace_back([func = std::forward<decltype(callable)>(callable)]() mutable -> ReturnType {
                return ReturnType { std::invoke(func) };
            }), ...);
        }, std::move(pack.values));
    }

}
