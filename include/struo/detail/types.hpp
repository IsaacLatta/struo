#pragma once

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

template<typename Domain>
struct ReferencesT {
    using domain_type = Domain;
};

template<auto V>
struct ValueT {
    [[nodiscard]] constexpr auto operator()() const noexcept {
        return V;
    }
};

template<typename... Domains>
struct DefinesT {};

struct DomainMetadata {
    std::string_view name{};
};

using DomainId = const DomainMetadata*;

struct InstanceTypeMetadata {
    bool (*equal)(const void*, const void*);
};

using InstanceTypeId = const InstanceTypeMetadata*;

struct DefinitionKey {
    DomainId domain_{};
    const void* addr_{};
    InstanceTypeId type_{};

    [[nodiscard]] bool operator==(const DefinitionKey& other) const {
        STRUO_ASSERT(domain_ && type_ && addr_, "attempt to compare incomplete definition key!");
        STRUO_ASSERT(other.domain_ && other.type_ && other.addr_, "attempt to compare incomplete definition key!");

        if(domain_ != other.domain_ || type_ != other.type_) {
            return false;
        }

        return type_->equal(addr_, other.addr_);
    }
};

struct Reference {
    DomainId domain_id_{};
    InstanceTypeId type_{};
};

template<typename T>
struct UnwrapOptional {
    using type = T;
};

template<typename T>
struct UnwrapOptional<std::optional<T>> : UnwrapOptional<T> {};

template<typename T>
using UnwrapOptionalT = typename UnwrapOptional<std::remove_cvref_t<T>>::type;

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

template<typename ValueType, typename ReturnType, typename Container, typename Tag, typename... Callables>
constexpr void apply_and_wrap_arg_func_pack(TaggedArgPack<Tag, Callables...> pack, Container& container) {
    std::apply([&](auto&&... callable){
        (container.emplace_back([func = std::forward<decltype(callable)>(callable)](auto&&... args) mutable -> ReturnType {
            if constexpr (std::invocable<decltype(func), decltype(args)...>) {
                return ReturnType { std::invoke(func, std::forward<decltype(args)>(args)...) };
            } else {
                return ReturnType { func.template operator()<ValueType>( std::forward<decltype(args)>(args)...) };
            }
        }), ...);
    }, std::move(pack.values));
}

}