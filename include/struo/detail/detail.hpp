#pragma once

#include <expected>
#include <string>
#include <filesystem>
#include <string_view>

#include "struo/Error.hpp"

namespace struo::detail {

    [[nodiscard]] constexpr std::expected<FileFormat, Error> file_format_from_path(const std::filesystem::path&);

    [[nodiscard]] constexpr std::expected<std::string, Error> file_to_string(const std::filesystem::path&);

}