#pragma once

#include <algorithm>
#include <cctype>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <utility>

#include "struo/Result.hpp"
#include "struo/concepts.hpp"
#include "struo/types.hpp"
#include "struo/detail/internal_concepts.hpp"

namespace struo::detail {

template<char... Chars>
struct TrimT {
    template<typename T>
    [[nodiscard]] constexpr T operator()(const T& in) const {
        constexpr auto predicate = [](char ch) { return ((ch == Chars) || ...); };
        if constexpr (std::same_as<T, std::filesystem::path>) {
            return std::filesystem::path{trim(in.string(), predicate)};
        } else {
            static_assert(std::constructible_from<T, std::string_view>, "TrimT requires T to be constructable from std::string_view");
            static_assert(std::constructible_from<std::string_view, const T&>, "TrimT requires std::string_view to be requires from T");
            return T{trim(std::string_view{in}, predicate)};
        }
    }

private:
    template<typename Predicate>
    [[nodiscard]] constexpr static std::string_view trim(std::string_view str, Predicate&& predicate) {
        const auto first = std::ranges::find_if_not(str, predicate);
        const auto last = std::ranges::find_if_not(str | std::views::reverse, predicate).base();
        return first == str.end() ? std::string_view{} : str.substr(first - str.begin(), last - first);
    }
};

template<Str Path>
struct RelativeToT {
    [[nodiscard]] std::string operator()(const std::string string) const {
        return makeRelative(std::filesystem::path{string}).string();
    }

    [[nodiscard]] std::filesystem::path operator()(const std::filesystem::path& path) const {
        return makeRelative(path);
    }

private:
    [[nodiscard]] std::filesystem::path makeRelative(const std::filesystem::path& path) const {
        return std::filesystem::path { Path.string } / path;
    }
};

struct ToLowerT {
    [[nodiscard]] std::string operator()(const std::string& string) const {
        return doToLower(string);
    }

    [[nodiscard]] std::filesystem::path operator()(const std::filesystem::path& path) const {
        return doToLower(path.string());
    }

private:
    [[nodiscard]] static std::string doToLower(const std::string& string) {
        auto copy { string };
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) { return std::tolower(c); });
        return copy;
    }
};

struct ToUpperT {
    [[nodiscard]] std::string operator()(const std::string& string) const {
        return doToUpper(string);
    }

    [[nodiscard]] std::filesystem::path operator()(const std::filesystem::path& path) const {
        return doToUpper(path.string());
    }

private:
    [[nodiscard]] static std::string doToUpper(const std::string& string) {
        auto copy { string };
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) { return std::toupper(c); });
        return copy;
    }
};

template<Str Prefix>
struct AddPrefixT {
    [[nodiscard]] constexpr std::string operator()(const std::string& string) const {
        if(string.empty()) {
            return string;
        }

        if(string.starts_with(Prefix.string)) {
            return string;
        }

        auto copy { string };
        copy.insert(size_t{0}, Prefix.string);
        return copy;
    }
};

template<auto... Inners>
struct IfPresentTransformT {
    template<typename T>
    requires (requires(const T& element) {
        Result<T>{std::invoke(Inners, element)};
    } && ...)
    constexpr Result<std::optional<T>> operator()(const std::optional<T>& value) const {
        if(!value) {
            return std::optional<T>{};
        }

        Result<T> result{*value};
        auto transform_one = [&](const auto& inner) {
            if(!result) {
                return;
            }
            result = Result<T>{std::invoke(inner, std::as_const(*result))};
        };

        (transform_one(Inners), ...);

        if(!result) {
            return err(result);
        }

        return std::optional<T>{std::move(*result)};
    }
};

template<auto... Inners>
struct ForEachTransformT {
    template<typename T>
    requires IsSequence<T> && (IsTransformFor<decltype(Inners), typename T::value_type> && ...) && std::default_initializable<T>
    constexpr Result<T> operator()(const T& value) const {
        T transformed{};
        size_t index{0u};
        for(const auto& element : value) {
            Result<typename T::value_type> result { element };
            auto transform_one = [&](const auto& inner) {
                if(result) {
                    result = Result<typename T::value_type> { std::invoke(inner, std::as_const(*result)) };
                }
            };

            (transform_one(Inners), ...);
            if(!result) {
                return err(result.error().code(),
                    std::format("\"for each\" transform failed: {} (on index={})", result.error().what(), index));
            }

            transformed.emplace_back(std::move(*result));
            ++index;
        }

        return transformed;
    }
};

} // namespace struo::detail
