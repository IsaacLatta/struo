#pragma once

#include <cstdlib>
#include <format>
#include <optional>
#include <string>

#include "struo/Result.hpp"
#include "struo/detail/detail.hpp"
#include "struo/detail/types.hpp"

namespace struo::detail {

template<Str Key>
struct FromEnvT {
    template<typename T>
    constexpr Result<std::optional<T>> operator()() const noexcept {
        const char* value_raw = std::getenv(Key.string);
        if(!value_raw) {
            return err(KEY_NOT_FOUND, std::format("env variable \"{}\" not set", Key.string));
        }
        std::string value_as_str { value_raw };

        auto value = detail::from_string<T>(value_as_str);
        if(!value) {
            return err(WRONG_TYPE, std::format("fail to convert env variable \"{}\" to type T", Key.string));
        }
        return std::optional<T>{*value};
    }
};

}

namespace struo {

template<typename Domain>
inline constexpr auto References { detail::ReferencesT<Domain>{} };

template<typename... Domains>
inline constexpr auto Defines { detail::DefinesT<Domains...>{} };

template<auto V>
static inline constexpr detail::ValueT<V> Value{};

template<Str Key>
static inline constexpr auto FromEnv { detail::FromEnvT<Key>{} };

}