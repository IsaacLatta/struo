#pragma once

#include <vector>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "struo/detail/internal_concepts.hpp"

namespace struo::detail {

template<auto... Inners>
struct IfPresentT {};

template<auto... Inners>
struct ForEachT {};

struct ConstraintOperation {};
struct TransformOperation {};

template<typename T>
inline constexpr bool always_false_v { false };

template<typename T>
struct MemberTraits;

template<typename Callable, typename... Args>
struct HasFunctionSignatureImpl : std::false_type {};

template<typename Callable, typename Return, typename... Args>
struct HasFunctionSignatureImpl<Callable, Return(Args...)> :
    std::bool_constant<requires(Callable& callable, Args... args) {
        { std::invoke(callable, std::forward<Args>(args)...) } -> std::same_as<Return>;
    }> {};

template<typename T>
struct IsChronoDurationImpl : std::false_type {};

template<typename Rep, typename Period>
struct IsChronoDurationImpl<std::chrono::duration<Rep, Period>> : std::true_type {};

template<typename T>
struct IsFieldImpl : std::false_type {};

template<typename T>
struct IsVariantImpl : std::false_type {};

template<typename... Ts>
struct IsVariantImpl<std::variant<Ts...>> : std::true_type {};

template<typename T>
struct IsOptionalImpl : std::false_type {};

template<typename T>
struct IsOptionalImpl<std::optional<T>> : std::true_type {};

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


}
