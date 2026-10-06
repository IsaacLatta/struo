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

#include "struo/detail/asserts.hpp"
#include "struo/detail/internal_concepts.hpp"

namespace struo {

template<auto... Inners>
struct IfPresentT {};

template<auto... Inners>
inline constexpr IfPresentT<Inners...> IfPresent{};

template<auto... Inners>
struct ForEachT {};

template<auto... Inners>
inline constexpr ForEachT<Inners...> ForEach{};

template <size_t N>
struct Str {
    char string[N];

    constexpr Str(const char (&str)[N]) {
        for (size_t i { 0 }; i < N; ++i)
            string[i] = str[i];
    }
};

}

namespace struo::detail {

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
    std::bool_constant<std::same_as<std::invoke_result_t<Callable&, Args...>, Return>> {};

template<typename T>
struct IsChronoDurationImpl : std::false_type {};

template<typename Rep, typename Period>
struct IsChronoDurationImpl<std::chrono::duration<Rep, Period>> : std::true_type {};

template<typename T>
struct IsVariantImpl : std::false_type {};

template<typename... Ts>
struct IsVariantImpl<std::variant<Ts...>> : std::true_type {};

template<typename T>
struct IsOptionalImpl : std::false_type {};

template<typename T>
struct IsOptionalImpl<std::optional<T>> : std::true_type {};

template<auto V>
struct ValueT {
    [[nodiscard]] constexpr auto operator()() const noexcept {
        return V;
    }
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


}
