#pragma once

#include <concepts>
#include <chrono>
#include <filesystem>
#include <ranges>
#include <type_traits>
#include <variant>
#include <optional>
#include <format>
#include <functional>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "struo/Result.hpp"

#include "struo/detail/types.hpp"

namespace struo {

template<typename T>
concept HasEmptyApi = requires(const T& t) {
    { t.empty() } -> std::convertible_to<bool>;
};

template<typename Callable, typename Value>
concept IsConstraintFor = requires(Callable& callable, const Value& value) {
    { std::invoke(callable, value) } -> std::same_as<Result<void>>;
};

template<typename Left, typename Right>
concept IsEqualityComparableWith = requires(const Left& left, const Right& right) {
    { left == right } -> std::convertible_to<bool>;
};

template<typename T>
concept HasSizeApi = requires(const T& value) {
    { value.size() } -> std::same_as<size_t>;
};

template<typename T>
concept HasFormatter = std::is_default_constructible_v<std::formatter<std::remove_cvref_t<T>, char>>;

template<auto Min, auto Max>
concept IsValidRangeBounds = std::same_as<decltype(Min), decltype(Max)> && requires { requires (Min <= Max); };

template<typename T>
concept IsOptional = detail::IsOptionalImpl<std::remove_cvref_t<T>>::value;

template<typename T>
concept IsStringLike = std::convertible_to<const T&, std::string_view>;

template<typename T>
concept IsField = detail::IsFieldImpl<std::remove_cvref_t<T>>::value;

template<typename T>
concept IsVariant = detail::IsVariantImpl<std::remove_cvref_t<T>>::value;

template<typename Callable, typename Signature>
concept HasFunctionSignature = detail::HasFunctionSignatureImpl<Callable, Signature>::value;

// Match the wrapper's result conversion and explicit value-type fallback.
template<typename Callable, typename Value, typename Return, typename... Args>
concept CanProduceResult = [] {
    if constexpr (std::invocable<Callable&, Args...>) {
        return requires(Callable& callable, Args... args) {
            Return{std::invoke(callable, std::forward<Args>(args)...)};
        };
    } else {
        return requires(Callable& callable, Args... args) {
            Return{callable.template operator()<Value>(std::forward<Args>(args)...)};
        };
    }
}();

template<typename T>
concept IsChronoDuration = detail::IsChronoDurationImpl<std::remove_cvref_t<T>>::value;

template<typename T>
struct SchemaTraits;

template<typename T>
concept IsPointerLike = std::is_pointer_v<T> || std::is_null_pointer_v<T>;

template<typename Subject, typename... Types>
concept IsOneOf = (0 + (std::same_as<std::remove_cvref_t<Subject>, std::remove_cvref_t<Types>> + ...) == 1);

template<typename Subject, typename... Args>
concept AppearsExactlyOnce = ((size_t{0} + ... + size_t{std::same_as<Subject, Args>}) == 1);

template<typename Subject, typename... Args>
concept AppearsAtMostOnce = ((size_t{0} + ... + size_t{std::same_as<Subject, Args>}) <= 1);

template<typename... Args>
concept AllAppearAtMostOnce = (AppearsAtMostOnce<std::remove_cvref_t<Args>, std::remove_cvref_t<Args>...> && ...);

template<typename T>
concept IsScalar = std::is_arithmetic_v<std::remove_cvref_t<T>> ||
    std::is_enum_v<std::remove_cvref_t<T>> ||
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    IsChronoDuration<T> ||
    std::same_as<std::remove_cvref_t<T>, std::filesystem::path>;

template<typename Callable, typename Value>
concept IsTransformFor = requires(Callable& callable, const Value& value) {
    Result<Value>{std::invoke(callable, value)};
};

template<typename T>
concept IsSequence = requires {
    typename T::value_type;
} && requires(T& array, typename T::value_type value) {
    array.emplace_back(value);
    { array.size() } -> std::convertible_to<size_t>;
} && !IsScalar<T> && std::ranges::input_range<const T>;

template<typename T>
concept IsMap = requires {
    typename T::key_type;
    typename T::mapped_type;
} && requires(T& map, typename T::key_type key, typename T::mapped_type value) {
    map.emplace(key, value);
    { map.size() } -> std::convertible_to<size_t>;
} && !IsSequence<T> && !IsScalar<T>;

template<typename T>
concept HasSchema = requires {
    SchemaTraits<T>::schema();
};

template<typename Parser, typename Value>
concept HasGetAsFor = requires(const std::remove_reference_t<Parser>& parser) {
    { parser.template getAs<Value>() } -> std::same_as<Result<Value>>;
};

template<typename T>
concept HasMemberTraits = requires {
    typename detail::MemberTraits<T>::value_type;
    typename detail::MemberTraits<T>::object_type;
};

template<typename T>
concept IsObject = HasSchema<T> && !IsVariant<T>;

template<typename T>
struct SupportedValue {
private:
    using underlying = std::remove_cvref_t<T>;

public:
    static constexpr bool value = []() {
        if constexpr (IsScalar<underlying> || IsObject<underlying>) {
            return true;
        } else if constexpr (IsVariant<underlying>) {
            return HasSchema<underlying> && []<typename... Ts>(std::type_identity<std::variant<Ts...>>) {
                return (SupportedValue<Ts>::value && ...);
            }(std::type_identity<underlying>{});
        } else if constexpr (IsSequence<underlying>) {
            return SupportedValue<typename underlying::value_type>::value;
        } else if constexpr (IsMap<underlying>) {
            return SupportedValue<typename underlying::key_type>::value && SupportedValue<typename underlying::mapped_type>::value;
        } else if constexpr (IsOptional<underlying>) {
            return SupportedValue<typename underlying::value_type>::value;
        } else {
            return false;
        }
    }();
};

template<typename T>
concept IsSupportedField = HasMemberTraits<T> && SupportedValue<typename detail::MemberTraits<T>::value_type>::value;

}
