#include "struo/detail/detail.hpp"
#include <fstream>
#include <algorithm>
#include <magic_enum/magic_enum.hpp>

namespace struo::detail {
    namespace {
        template<typename T>
        [[nodiscard]] constexpr std::string to_enum_names_fmt_str() {
            std::string out{};
            for (std::string_view name : magic_enum::enum_names<T>()) {
                out.append(std::format("{}, ", name));
            }
            return out.length() > 2 ? out.substr(0, out.length() - 2) : out;
        }
    }

constexpr std::expected<FileFormat, Error> file_format_from_path(const std::filesystem::path& path) {
    auto extension = path.extension().string();
    if (extension.empty()) {
        return err(INVALID_ARGUMENT, std::format("no file extension for file \"{}\"", path.string()));
    }

    if (extension.front() == '.') {
        extension.erase(extension.begin());
    }

    const auto ext = magic_enum::enum_cast<FileFormat>(extension);
    if (!ext) {
        auto enum_names = to_enum_names_fmt_str<FileFormat>();
        std::ranges::transform(enum_names, enum_names.begin(), [](auto c) { return static_cast<char>(std::tolower(c)); });
        return err(INVALID_ARGUMENT, std::format("unsupported file extension \"{}\", expected one of: {}", extension, enum_names));
    }

    return ext.value();
}

constexpr std::expected<std::string, Error> file_to_string(const std::filesystem::path& path) {
    std::error_code ec{};
    if (!std::filesystem::exists(path, ec)) {
        return err(FILE_NOT_FOUND, std::format("file \"{}\" does not exist", path.string()));
    }

    if (ec) {
        return err(UNKNOWN_ERROR, std::format("failed to stat file \"{}\": {}", path.string(), ec.message()));
    }

    std::ifstream stream { path };
    if (!stream) {
        return err(OPEN_FILE_FAILED, std::format("failed to open file \"{}\"", path.string()));
    }

    std::string contents { std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{} };

    if (stream.bad()) {
        return err(READ_FILE_FAILED, std::format("failed to read file \"{}\"", path.string()));
    }

    return contents;
}

}