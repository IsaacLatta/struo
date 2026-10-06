#pragma once

#include <concepts>
#include <memory>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>
#include <type_traits>
#include <variant>
#include <optional>
#include <array>

#include "struo/forward.hpp"
#include "struo/concepts.hpp"
#include "struo/detail/asserts.hpp"

#include "struo/detail/types.hpp"
#include "struo/detail/internal_concepts.hpp"
#include "struo/transforms/transforms.hpp"

namespace struo::detail {

template<typename Operation, typename Keyword>
struct KeywordTraits {
    using callable_type = Keyword;
    static constexpr callable_type adapt(Keyword keyword) {
        return keyword;
    }
};

template<typename Operation, auto Keyword>
consteval auto adapter_of() {
    using keyword_type = std::remove_cvref_t<decltype(Keyword)>;
    return KeywordTraits<Operation, keyword_type>::adapt(Keyword);
}

template<auto... Inners>
struct KeywordTraits<TransformOperation, IfPresentT<Inners...>> {
    using callable_type  = IfPresentTransformT<adapter_of<TransformOperation, Inners>()...>;
    static constexpr callable_type adapt(IfPresentT<Inners...>) {
        return {};
    }
};

template<auto... Inners>
struct KeywordTraits<ConstraintOperation, ForEachT<Inners...>> {
    using callable_type = ForEachConstraint<adapter_of<ConstraintOperation, Inners>()...>;
    static constexpr callable_type adapt(ForEachT<Inners...>) {
        return {};
    }
};

template<auto... Inners>
struct KeywordTraits<TransformOperation, ForEachT<Inners...>> {
    using callable_type = ForEachTransformT<adapter_of<TransformOperation, Inners>()...>;
    static constexpr callable_type adapt(ForEachT<Inners...>) {
        return {};
    }
};

template<typename ValueType, typename ReturnType, typename Operation = void, typename Container, typename Tag, typename... Callables>
constexpr void apply_and_wrap_arg_func_pack(TaggedArgPack<Tag, Callables...> pack, Container& container) {
    std::apply([&](auto&&... callable){
        (container.emplace_back([func = KeywordTraits<Operation, std::remove_cvref_t<decltype(callable)>>::adapt(
            std::forward<decltype(callable)>(callable))](auto&&... args) mutable -> ReturnType {
            if constexpr (std::invocable<decltype(func), decltype(args)...>) {
                return ReturnType { std::invoke(func, std::forward<decltype(args)>(args)...) };
            } else {
                return ReturnType { func.template operator()<ValueType>( std::forward<decltype(args)>(args)...) };
            }
        }), ...);
    }, std::move(pack.values));
}

template<typename Object, typename Value>
struct MemberTraits<Value Object::*> {
    using value_type = Value;
    using object_type = Object;
};

template<typename T>
struct ValueTraits;

template<typename T>
requires IsMap<T>
struct ValueTraits<T> {
    using key_type = typename T::key_type;
    using mapped_type = typename T::mapped_type;
    using staged_key_type = typename ValueTraits<key_type>::staged_type;
    using staged_mapped_type = typename ValueTraits<mapped_type>::staged_type;
    using staged_type = std::vector<std::pair<staged_key_type, staged_mapped_type>>;
};

template<typename T>
requires IsSequence<T>
struct ValueTraits<T> {
    using element_type = typename T::value_type;
    using staged_element_type = typename ValueTraits<element_type>::staged_type;
    using staged_type = std::vector<staged_element_type>;
};

template<typename T>
requires IsScalar<T>
struct ValueTraits<T> {
    using value_type = T;
    using staged_type = value_type;
};

template<typename T>
requires IsObject<T>
struct ValueTraits<T> {
    using schema_type = decltype(SchemaTraits<T>::schema());
    using staged_type = schema_type;
};

template<typename T>
struct ValueTraits<std::optional<T>> {
    using value_type = T;
    using staged_type = ValueTraits<T>::staged_type;
};

template<typename... Ts>
struct ValueTraits<std::variant<Ts...>> {
    using value_type = std::variant<Ts...>;
    using schema_type = decltype(SchemaTraits<value_type>::schema());
    using staged_type = std::variant<typename ValueTraits<Ts>::staged_type...>;

    template<typename T>
    requires AppearsExactlyOnce<T, Ts...>
    static constexpr size_t index_of = []() {
        constexpr bool matches[] = { std::same_as<T, Ts>... };
        for(size_t i { 0u }; i < sizeof...(Ts); ++i) {
            if(matches[i]) {
                return i;
            }
        }
        return std::numeric_limits<size_t>::max();
    }();

private:
    using bindings_type = typename schema_type::bindings_type;

    template<typename... Bs>
    static constexpr bool all_alternatives_are_bound_once(std::tuple<Bs...>) {
        return (AppearsExactlyOnce<Ts, typename Bs::value_type...> && ...);
    }

    template<typename... Bs>
    static constexpr bool all_bindings_bind_to_variant_alternative(std::tuple<Bs...>) {
        return (AppearsExactlyOnce<typename Bs::value_type, Ts...> && ...);
    }

public:
    static_assert(
        all_alternatives_are_bound_once(bindings_type{}),
        "Variant schema must bind each destination alternative exactly once");

    static_assert(
        all_bindings_bind_to_variant_alternative(bindings_type{}),
        "Each variant binding must be bound to exactly one destination alternative");
};

}
