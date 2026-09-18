#pragma once

#include <concepts>
#include <type_traits>
#include <variant>
#include <optional>

#include "struo/forward.hpp"
#include "struo/concepts.hpp"

namespace struo::detail {

    template<typename T>
    concept IsStringLike = std::convertible_to<const T&, std::string_view>;

    template<typename Domain>
    struct ReferencesImpl {
        using domain_type = Domain;
    };

    template<typename... Domains>
    struct DefinesImpl {};

    template<typename T>
    concept HasName = requires {
        { T::name() } -> std::convertible_to<std::string_view>;
    };

    template<typename T>
    concept HasDescription = requires {
        { T::description() } -> std::convertible_to<std::string_view>;
    };

    template<typename T>
    concept HasDomainTraits = requires { sizeof(DomainTraits<T>); };

    template<typename T>
    [[nodiscard]] constexpr std::string_view domain_name() {
        if constexpr (HasDomainTraits<T> && HasName<DomainTraits<T>>) {
            return DomainTraits<T>::name();
        } else {
            return "<unnamed-domain>";
        }
    }

    struct DomainMetadata {
        std::string_view name{};
    };

    template<typename T>
    inline constexpr DomainMetadata DomainMeta { domain_name<T>() };

    using DomainId = DomainMetadata*;

    template<typename T>
    inline constexpr DomainId DomainIdOf { &DomainMeta<T> };

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
