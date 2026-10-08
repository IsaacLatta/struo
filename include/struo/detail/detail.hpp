#pragma once

#include <array>
#include <string>
#include <string_view>
#include <charconv>
#include <concepts>
#include <optional>
#include <type_traits>
#include <filesystem>

#include <magic_enum/magic_enum.hpp>

#include "struo/concepts.hpp"
#include "struo/Result.hpp"
#include "struo/detail/traits.hpp"

namespace struo::detail {

    template<typename... T>
    concept AreMutableLValueReferences = ((!std::is_const_v<std::remove_reference_t<T>> && std::is_lvalue_reference_v<T>) && ...);

    template<typename T>
    using SchemaStage = decltype(SchemaTraits<T>::schema());

    template <typename T>
    std::optional<T> from_string(std::string_view str) {
        T value{};
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
        if (ec != std::errc{} || ptr != str.data() + str.size()) {
            return std::nullopt;
        }
        return value;
    }

    template<typename T>
    requires std::is_enum_v<T>
    [[nodiscard]] constexpr std::string_view enum_name(T t) {
        return magic_enum::enum_name(t);
    }

    template<typename T>
    requires std::is_enum_v<T> || IsStringLike<T>
    [[nodiscard]] constexpr std::string_view as_string(T value) {
        if constexpr (std::is_enum_v<T>) {
            return detail::enum_name(value);
        } else {
            return value;
        }
    }

    template<typename T>
    [[nodiscard]] constexpr std::string_view name_of() {
        using U = std::remove_cvref_t<T>;
        if constexpr (HasName<U>) {
            return U::name();
        } else {
            return "<unnamed>";
        }
    }

    template<typename T>
    [[nodiscard]] constexpr std::string description_of() {
        using U = std::remove_cvref_t<T>;
        if constexpr (HasDescription<U>) {
            return std::string{U::description()};
        } else {
            return {};
        }
    }

    template<typename... Bs>
    [[nodiscard]] constexpr bool all_unique_binding_tags() {
        constexpr std::array<std::string_view, sizeof...(Bs)> names { Bs::tag... };
        for(size_t i { 0 }; i < names.size(); ++i) {
            for(size_t j { i + 1 }; j < names.size(); ++j) {
                if(names[i] == names[j]) {
                    return false;
                }
            }
        }
        return true;
    }

    template<typename Enum, typename Parser>
    requires std::is_enum_v<Enum>
    [[nodiscard]] constexpr Result<Enum> get_as_enum(const Parser& parser) {
        auto result = parser.template getAs<std::string>();
        if(!result) {
            return err(result);
        }

        auto value = magic_enum::enum_cast<Enum>(result.value(), magic_enum::case_insensitive);
        if(!value) {
            return err(INVALID_VALUE, std::format("\"{}\" is invalid", *result));
        }

        return *value;
    }

    template<typename T, typename Parser>
    [[nodiscard]] constexpr Result<T> get_as_chrono_duration(const Parser& parser) {
        auto count = parser.template getAs<typename T::rep>();
        if(!count) {
            return err(count);
        }
        return T{*count};
    }

    template<typename Parser>
    [[nodiscard]] constexpr Result<std::filesystem::path> get_as_path(const Parser& parser) {
        auto text = parser.template getAs<std::string>();
        if(!text) {
            return err(text);
        }

        try {
            return std::filesystem::path{std::u8string(text.value().begin(), text.value().end())};
        } catch(const std::filesystem::filesystem_error& e) {
            return err(INVALID_VALUE, e.what());
        }
    }

    template<typename Predicate>
    requires std::predicate<Predicate, char>
    [[nodiscard]] constexpr std::string_view trim(const std::string_view& str, Predicate&& predicate) {
        const auto first = std::ranges::find_if_not(str, predicate);
        const auto last = std::ranges::find_if_not(str | std::views::reverse, predicate).base();
        return first == str.end() ? std::string_view{} : str.substr(first - str.begin(), last - first);
    }

    template<typename Predicate>
    requires std::predicate<Predicate, char>
    [[nodiscard]] std::filesystem::path trim(const std::filesystem::path& path, Predicate&& predicate) {
        return std::filesystem::path{trim(path.string(), std::forward<Predicate>(predicate))};
    }

    template<typename T, typename Predicate>
    requires std::predicate<Predicate, char> &&
             std::constructible_from<std::string_view, const T&> &&
             std::constructible_from<T, std::string_view>
    [[nodiscard]] constexpr T trim(const T& in, Predicate&& predicate) {
        return T{trim(std::string_view{in}, std::forward<Predicate>(predicate))};
    }
}
