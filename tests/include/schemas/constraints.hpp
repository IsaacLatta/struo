#pragma once

#include "struo/struo.hpp"
#include <chrono>
#include <optional>

namespace test_schemas {

inline constexpr auto PositiveDuration = [](std::chrono::milliseconds value) {
    return struo::Positive(value.count());
};

inline constexpr auto OptionalPositiveDuration = [](const std::optional<std::chrono::milliseconds>& value) {
    return value ? PositiveDuration(*value) : struo::ok();
};

inline constexpr auto PositiveMultiplier = struo::Positive;

}
