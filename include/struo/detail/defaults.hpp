#pragma once

#include <cstdlib>
#include <format>
#include <optional>
#include <string>

#include "struo/Result.hpp"
#include "struo/detail/detail.hpp"
#include "struo/types.hpp"

namespace struo::detail {

template<auto V>
struct ValueT {
    [[nodiscard]] constexpr auto operator()() const noexcept {
        return V;
    }
};

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

} // namespace struo::detail
