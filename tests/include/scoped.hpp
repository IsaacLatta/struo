#pragma once

#include <filesystem>
#include <random>
#include <format>
#include <iostream>
#include <fstream>

namespace struo::testlib {

    [[nodiscard]] inline std::string clean_extension(std::string_view ext) {
        if (ext.empty() || ext.starts_with('.')) {
            return std::string{ext};
        }

        return std::format(".{}", ext);
    }

    [[nodiscard]] inline std::filesystem::path create_unique_name() {
        static thread_local std::mt19937_64 rng{std::random_device{}()};
        const uint64_t high = rng();
        const uint64_t low = rng();
        return std::filesystem::path { std::format("struo_test_file_{}_{}", high, low) };
    }

    [[nodiscard]] inline std::filesystem::path create_unique_file_name(const std::filesystem::path& parent_dir, const std::string& extension) {
        return parent_dir / std::format("{}{}", create_unique_name().string(), clean_extension(extension));
    }

    class ScopedFile {
    public:
        explicit ScopedFile(std::string_view contents, const std::string& extension)
            : path_{create_unique_file_name(std::filesystem::temp_directory_path(), extension)} {
                tryWriteContents(contents);
            }

        ~ScopedFile() noexcept {
            std::error_code ignored{};
            (void)std::filesystem::remove(path_, ignored);
        }

        [[nodiscard]] const std::filesystem::path& getPath() const noexcept {
            return path_;
        }

    private:
        void tryWriteContents(std::string_view contents) {
            std::filesystem::create_directories(path_.parent_path());
            std::ofstream file{ path_, std::ios::binary | std::ios::trunc };
            file.exceptions(std::ios::failbit | std::ios::badbit);
            file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        }

    private:
        std::filesystem::path path_;
    };

    class ScopedDirectory {
    public:
        ScopedDirectory() : path_{std::filesystem::temp_directory_path() / create_unique_name()} {
            tryCreate();
        }

        ~ScopedDirectory() noexcept {
            std::error_code ignored{};
            (void)std::filesystem::remove_all(path_, ignored);
        }

        [[nodiscard]] const std::filesystem::path& getPath() const noexcept {
            return path_;
        }

    private:
        void tryCreate() const {
            if (!std::filesystem::create_directory(path_)) {
                throw std::runtime_error { std::format("failed to create test directory '{}'", path_.string()) };
            }
        }

    private:
        std::filesystem::path path_;
    };


}
