#pragma once

#include <tuple>
#include <concepts>

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

    template<typename Container, typename Tag, typename... Args>
    constexpr void apply_arg_pack(TaggedArgPack<Tag, Args...>&& pack, Container& container) {
        std::apply([&](auto&&... values) {
            (container.emplace_back(std::move(values)), ...);
        }, std::move(pack.values));
    }
}
