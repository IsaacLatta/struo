#include "struo/detail/detail.hpp"

#include <fstream>
#include <algorithm>
#include <magic_enum/magic_enum.hpp>

namespace struo::detail {

Result<FileFormat> file_format_from_path(const std::filesystem::path& path) {
    auto extension = path.extension().string();
    if (extension.empty()) {
        return err(INVALID_ARGUMENT, std::format("no file extension for file \"{}\"", path.string()));
    }

    if (extension.front() == '.') {
        extension.erase(extension.begin());
    }

    const auto ext = magic_enum::enum_cast<FileFormat>(extension);
    if (!ext) {
        return err(INVALID_ARGUMENT, std::format("unsupported file extension \"{}\"", extension));
    }

    return ext.value();
}

}