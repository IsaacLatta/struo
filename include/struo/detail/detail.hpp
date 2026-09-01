#pragma once

#include <expected>
#include <string>
#include <filesystem>
#include <string_view>
#include <magic_enum/magic_enum.hpp>

#include "struo/Error.hpp"

namespace struo::detail {

    template<typename T>
    requires std::is_enum_v<T>
    [[nodiscard]] constexpr std::string_view enum_name(T t) {
        return magic_enum::enum_name(t);
    }

    [[nodiscard]] constexpr std::expected<FileFormat, Error> file_format_from_path(const std::filesystem::path&);

    [[nodiscard]] constexpr std::expected<std::string, Error> file_to_string(const std::filesystem::path&);

}