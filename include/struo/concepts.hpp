#pragma once

#include <concepts>
#include <chrono>

namespace struo {
    namespace detail {
        template<typename T>
        struct MemberTraits;

        template<typename Callable, typename... Args>
        struct HasFunctionSignatureImpl : std::false_type {};

        template<typename Callable, typename Return, typename... Args>
        struct HasFunctionSignatureImpl<Callable, Return(Args...)> :
            std::bool_constant<std::same_as<std::invoke_result_t<Callable&, Args...>, Return>> {};

        template<typename T>
        struct IsVariantImpl : std::false_type {};

        template<typename... Ts>
        struct IsVariantImpl<std::variant<Ts...>> : std::true_type {};
    }

    template<typename T>
    concept IsStringLike = std::convertible_to<const T&, std::string_view>;

    template<typename T>
    concept IsVariant = detail::IsVariantImpl<std::remove_cvref_t<T>>::value;

    template<typename Callable, typename Signature>
    concept HasFunctionSignature = detail::HasFunctionSignatureImpl<Callable, Signature>::value;

    template<typename T>
    struct IsChronoDuration : std::false_type {};

    template<typename Rep, typename Period>
    struct IsChronoDuration<std::chrono::duration<Rep, Period>> : std::true_type {};

    template<typename T>
    struct SchemaTraits;

    template<typename Subject, typename... Types>
    concept IsOneOf = (0 + (std::same_as<std::remove_cvref_t<Subject>, std::remove_cvref_t<Types>> + ...) == 1);

    template<typename T>
    concept IsPointerToMember = true;

    template<typename T>
    concept IsScalar = std::is_arithmetic_v<std::remove_cvref_t<T>> ||
        std::is_enum_v<std::remove_cvref_t<T>> ||
        std::same_as<std::remove_cvref_t<T>, std::string> ||
        IsChronoDuration<T>::value_type;

    template<typename T>
    concept IsSequence = requires {
        typename T::value_type;
    } && requires(T& array, typename T::value_type value) {
        array.emplace_back(value);
        { array.size() } -> std::convertible_to<size_t>;
    } && !IsScalar<T> ;

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
            } else if constexpr (IsSequence<underlying>) {
                return SupportedValue<typename underlying::value_type>::value;
            } else if constexpr (IsMap<underlying>) {
                return SupportedValue<typename underlying::key_type>::value && SupportedValue<typename underlying::mapped_type>::value;
            } else {
                return false;
            }
        }();
    };

    template<typename T>
    concept IsSupportedField = HasMemberTraits<T> && SupportedValue<typename detail::MemberTraits<T>::value_type>::value;
}
