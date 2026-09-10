#pragma once

#include <type_traits>
#include <limits>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include "struo/Error.hpp"
#include "struo/Result.hpp"
#include "struo/constraints/range.hpp"

#include "struo/detail/platform.hpp"

namespace struo {

    struct FileExistsConstraint {
        static constexpr std::string_view name() {
            return "file exists";
        }

        static constexpr std::string_view description() {
            return "path must exist";
        }

        template<typename Path>
        requires std::constructible_from<std::filesystem::path, const Path&>
        Result<void> operator()(const Path& file_path) const {
            try {
                std::filesystem::path path{ file_path };

                std::error_code ec{};
                const bool exists = std::filesystem::exists(path, ec);

                if(ec) {
                    return err(UNKNOWN_ERROR, std::format("\"{}\" constraint failed: failed to stat file \"{}\": {}", name(), path.string(), ec.message()));
                }

                if(!exists) {
                    return err(FILE_NOT_FOUND, std::format("\"{}\" constraint failed: file \"{}\" does not exist", name(), path.string()));
                }

                return ok();
            } catch(const std::filesystem::filesystem_error& e) {
                return err(UNKNOWN_ERROR, std::format("\"{}\" constraint failed: failed to construct or inspect filesystem path: {}", name(), e.what()));
            }
        }
    };

    struct DirectoryExistsConstraint {
        static constexpr std::string_view name() {
            return "directory exists";
        }

        static constexpr std::string_view description() {
            return "path must refer to an existing directory";
        }

        template<typename Path>
        requires std::constructible_from<std::filesystem::path, const Path&>
        Result<void> operator()(const Path& file_path) const {
            try {
                std::filesystem::path path { file_path };
                auto exists = FileExistsConstraint{}(path);
                if(!exists) {
                    return err(exists.error().code(), std::format("\"{}\" constraint failed: {}", name(), exists.error().what()));
                }

                std::error_code ec{};
                const bool is_directory = std::filesystem::is_directory(path, ec);
                if(ec) {
                    return err(UNKNOWN_ERROR, std::format("\"{}\" constraint failed: failed to stat file \"{}\": {}", name(), path.string(), ec.message()));
                }

                if(!is_directory) {
                    return err(INVALID_ARGUMENT, std::format("\"{}\" constraint failed: file \"{}\" is not a directory", name(), path.string()));
                }

                return ok();
            } catch(const std::filesystem::filesystem_error& e) {
                return err(UNKNOWN_ERROR, std::format("\"{}\" constraint failed: failed to construct or inspect filesystem path: {}", name(), e.what()));
            }
        }
    };

    inline constexpr auto FileExists { FileExistsConstraint{} };

    inline constexpr auto DirectoryExists { DirectoryExistsConstraint{} };

    inline constexpr auto IsValidPort { Range<0, 65535> };

#if STRUO_PLATFORM_LINUX

    struct IsValidHostnameConstraint {
        static constexpr std::string_view name() {
            return "hostname";
        }

        static constexpr std::string_view description() {
            return "must be a valid hostname";
        }

        Result<void> operator()(const std::string& host_name) const {
            auto result = detail::is_valid_hostname(host_name);
            if(!result) {
                return err(result.error().code(),
                    std::format("\"{}\" constraint failed: {}", name(), result.error().what()));
            }
            return ok();
        }
    };

    struct IsValidIpv4Constraint {
        static constexpr std::string_view name() {
            return "IPv4 address";
        }

        static constexpr std::string_view description() {
            return "must be a valid IPv4 address";
        }

        Result<void> operator()(const std::string& address) const {
            auto result = detail::is_valid_ipv4(address);
            if(!result) {
                return err(result.error().code(),
                    std::format("\"{}\" constraint failed: {}", name(), result.error().what()));
            }
            return ok();
        }
    };

    struct IsValidIpv6Constraint {
        static constexpr std::string_view name() {
            return "IPv6 address";
        }

        static constexpr std::string_view description() {
            return "must be a valid IPv6 address";
        }

        Result<void> operator()(const std::string& address) const {
            auto result = detail::is_valid_ipv6(address);
            if(!result) {
                return err(result.error().code(),
                    std::format("\"{}\" constraint failed: {}", name(), result.error().what()));
            }
            return ok();
        }
    };

    inline constexpr auto IsValidHostname { IsValidHostnameConstraint{} };

    inline constexpr auto IsValidIpv6 { IsValidIpv6Constraint{} };

    inline constexpr auto IsValidIpv4 { IsValidIpv4Constraint{} };

#endif // STRUO_PLATFORM_LINUX

}
