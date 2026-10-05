#pragma once

#include "struo/struo.hpp"
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace test_schemas {

// Range constraints format their operands as numbers; duration constraints
// validate the count while retaining chrono types in the configuration.
inline constexpr auto PositiveDuration = [](std::chrono::milliseconds value) {
    return struo::AtLeast<1LL>(value.count());
};

inline constexpr auto OptionalPositiveDuration = [](const std::optional<std::chrono::milliseconds>& value) {
    return value ? PositiveDuration(*value) : struo::ok();
};

inline constexpr auto NonemptyPath = [](const std::filesystem::path& value) {
    return value.empty() ? struo::err(struo::INVALID_VALUE, "path must be nonempty") : struo::ok();
};

inline constexpr auto HttpPath = [](const std::string& value) {
    return value.starts_with('/') ? struo::ok() : struo::err(struo::INVALID_VALUE, "HTTP path must start with /");
};

inline constexpr auto PositiveMultiplier = [](double value) {
    return value > 0.0 ? struo::ok() : struo::err(struo::INVALID_VALUE, "speed multiplier must be positive");
};

} // namespace test_schemas
