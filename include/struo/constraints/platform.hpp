#pragma once

#include <type_traits>
#include <limits>
#include <filesystem>

#include "struo/Error.hpp"
#include "struo/Result.hpp"
#include "struo/constraints/range.hpp"

#include "struo/detail/platform.hpp"

namespace struo {

    struct FileExistsConstraint {
        template<typename Path>
        requires std::constructible_from<std::filesystem::path, const Path&>
        Result<void> operator()(const Path& file_path) const {
            try {
                std::filesystem::path path{ file_path };

                std::error_code ec{};
                const bool exists = std::filesystem::exists(path, ec);

                if(ec) {
                    return err(UNKNOWN_ERROR, std::format("failed to stat file \"{}\": {}", path.string(), ec.message()));
                }

                if(!exists) {
                    return err(FILE_NOT_FOUND, std::format("file \"{}\" does not exist", path.string()));
                }

                return ok();
            } catch(const std::filesystem::filesystem_error& e) {
                return err(UNKNOWN_ERROR, std::format("failed to construct or inspect filesystem path: {}", e.what()));
            }
        }
    };

    struct DirectoryExistsConstraint {
        template<typename Path>
        requires std::constructible_from<std::filesystem::path, const Path&>
        Result<void> operator()(const Path& file_path) const {
            try {
                std::filesystem::path path { file_path };
                auto exists = FileExistsConstraint{}(path);
                if(!exists) {
                    return err(exists);
                }

                std::error_code ec{};
                const bool is_directory = std::filesystem::is_directory(path, ec);
                if(ec) {
                    return err(UNKNOWN_ERROR, std::format("failed to stat file \"{}\": {}", path.string(), ec.message()));
                }

                if(!is_directory) {
                    return err(INVALID_ARGUMENT, std::format("file \"{}\" is not a directory", path.string()));
                }

                return ok();
            } catch(const std::filesystem::filesystem_error& e) {
                return err(UNKNOWN_ERROR, std::format("failed to construct or inspect filesystem path: {}", e.what()));
            }
        }
    };

    inline constexpr auto FileExists { FileExistsConstraint{} };

    inline constexpr auto DirectoryExists { DirectoryExistsConstraint{} };

    inline constexpr auto IsValidPort { Range<0, 65535> };

#if STRUO_PLATFORM_LINUX

    struct IsValidHostnameConstraint {
        Result<void> operator()(const std::string& host_name) const {
            return detail::is_valid_hostname(host_name);
        }
    };

    struct IsValidIpv4Constraint {
        Result<void> operator()(const std::string& address) const {
            return detail::is_valid_ipv4(address);
        }
    };

    struct IsValidIpv6Constraint {
        Result<void> operator()(const std::string& address) const {
            return detail::is_valid_ipv6(address);
        }
    };

    inline constexpr auto IsValidHostname { IsValidHostnameConstraint{} };

    inline constexpr auto IsValidIpv6 { IsValidIpv6Constraint{} };

    inline constexpr auto IsValidIpv4 { IsValidIpv4Constraint{} };

#endif // STRUO_PLATFORM_LINUX

}
