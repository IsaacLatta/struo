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

#include "struo/Result.hpp"
#include "struo/detail/parsers/JsonParser.hpp"
#include "struo/detail/parsers/TomlParser.hpp"
#include "struo/detail/parsers/YamlParser.hpp"

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

template<>
struct FormatReader<YamlParser> {
    using Document = YAML::Node;

    [[nodiscard]] static Result<Document> parse(std::string_view contents) {
        try {
            return YAML::Load(std::string{contents});
        } catch (const YAML::Exception& error) {
            return err(PARSE_ERROR, error.what());
        }
    }

    [[nodiscard]] static YamlParser makeParser(Document& document) {
        return YamlParser{document};
    }
};

template<>
struct FormatReader<JsonParser> {
    using Document = nlohmann::json;

    [[nodiscard]] static Result<Document> parse(std::string_view contents) {
        try {
            return nlohmann::json::parse(contents.begin(), contents.end());
        } catch (const nlohmann::json::exception& error) {
            return err(PARSE_ERROR, error.what());
        }
    }

    [[nodiscard]] static JsonParser makeParser(Document& document) {
        return JsonParser{std::move(document)};
    }
};

template<>
struct FormatReader<TomlParser> {
    using Document = toml::table;

    [[nodiscard]] static Result<Document> parse(std::string_view contents) {
        try {
            return toml::parse(contents);
        } catch (const toml::parse_error& error) {
            return err(PARSE_ERROR, std::string{error.description()});
        }
    }

    [[nodiscard]] static TomlParser makeParser(Document& document) {
        return TomlParser{document};
    }
};

template<typename Format>
concept HasFormatReader = requires(std::string_view contents, typename FormatReader<Format>::Document& document) {
    { FormatReader<Format>::parse(contents) } -> std::same_as<Result<typename FormatReader<Format>::Document>>;
    { FormatReader<Format>::makeParser(document) } -> std::same_as<Format>;
};

} // namespace struo::detail
