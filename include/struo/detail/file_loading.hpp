#pragma once

#include <array>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <istream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "struo/forward.hpp"
#include "struo/Result.hpp"

namespace struo::detail {

[[nodiscard]] inline Result<std::string> read_stream(std::istream& input) {
    std::string contents;
    std::array<char, 8192> buffer{};

    for (;;) {
        try {
            input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        } catch (const std::ios_base::failure&) {
            if (input.bad() || !input.eof()) {
                return err(READ_FILE_FAILED, "failed to read file contents");
            }
        }

        contents.append(buffer.data(), static_cast<size_t>(input.gcount()));
        if (input.bad() || (input.fail() && !input.eof())) {
            return err(READ_FILE_FAILED, "failed to read file contents");
        }
        if (input.eof()) {
            return contents;
        }
    }
}

[[nodiscard]] inline Result<std::string> read_file(const std::filesystem::path& path) {
    std::error_code ec;
    if (std::filesystem::is_directory(path, ec)) {
        return err(OPEN_FILE_FAILED, "cannot open directory as a configuration file: " + path.string());
    }

    std::ifstream input{path, std::ios::binary};
    if (!input.is_open()) {
        const auto status = std::filesystem::status(path, ec);
        if (status.type() == std::filesystem::file_type::not_found) {
            return err(FILE_NOT_FOUND, "configuration file not found: " + path.string());
        }
        return err(OPEN_FILE_FAILED, "failed to open configuration file: " + path.string());
    }

    auto contents = read_stream(input);
    if (!contents) {
        return err(READ_FILE_FAILED, "failed to read configuration file: " + path.string());
    }
    return contents;
}

template<typename Format>
struct FormatReader;

template<typename Format>
concept HasFormatReader = requires(std::string_view contents, typename FormatReader<Format>::Document& document) {
    { FormatReader<Format>::parse(contents) } -> std::same_as<Result<typename FormatReader<Format>::Document>>;
    { FormatReader<Format>::makeParser(document) } -> std::same_as<Format>;
};

} // namespace struo::detail
