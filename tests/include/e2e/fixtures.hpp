#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include "struo/struo.hpp"

namespace e2e {

struct YamlFormat {
    using Format = struo::Yaml;
    static constexpr std::string_view extension = ".yaml";

    static ::testing::AssertionResult validateSyntax(std::string_view contents) {
        try {
            (void)YAML::Load(std::string{contents});
            return ::testing::AssertionSuccess();
        } catch (const YAML::Exception& error) {
            return ::testing::AssertionFailure() << error.what();
        }
    }
};

struct JsonFormat {
    using Format = struo::Json;
    static constexpr std::string_view extension = ".json";

    static ::testing::AssertionResult validateSyntax(std::string_view contents) {
        try {
            const auto document = nlohmann::json::parse(contents.begin(), contents.end());
            (void)document;
            return ::testing::AssertionSuccess();
        } catch (const nlohmann::json::exception& error) {
            return ::testing::AssertionFailure() << error.what();
        }
    }
};

struct TomlFormat {
    using Format = struo::Toml;
    static constexpr std::string_view extension = ".toml";

    static ::testing::AssertionResult validateSyntax(std::string_view contents) {
        try {
            (void)toml::parse(contents);
            return ::testing::AssertionSuccess();
        } catch (const toml::parse_error& error) {
            return ::testing::AssertionFailure() << error.description();
        }
    }
};

// Both scenario suites register against this single list.
using AllFormats = ::testing::Types<YamlFormat, JsonFormat, TomlFormat>;

template<typename Descriptor>
inline std::filesystem::path fixture_path(std::string_view scenario, std::string_view stem) {
    return std::filesystem::path{STRUO_TEST_FIXTURE_ROOT} / scenario /
        (std::string{stem} + std::string{Descriptor::extension});
}

inline struo::Result<std::string> read_fixture(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    if (!input.is_open()) {
        return struo::err(struo::OPEN_FILE_FAILED, "cannot read fixture: " + path.string());
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    if (input.bad() || contents.bad()) {
        return struo::err(struo::READ_FILE_FAILED, "failed to read fixture: " + path.string());
    }
    return contents.str();
}

template<typename Config, typename Descriptor>
class FixtureTest : public ::testing::Test {
protected:
    struo::Result<Config> loadCase(std::string_view scenario, std::string_view stem, bool expect_valid_syntax = true) {
        const auto path = fixture_path<Descriptor>(scenario, stem);
        SCOPED_TRACE(path.string());

        auto contents = read_fixture(path);
        EXPECT_TRUE(contents) << contents.error().what();
        if (!contents) {
            return struo::err(contents);
        }

        const auto syntax = Descriptor::validateSyntax(*contents);
        EXPECT_EQ(static_cast<bool>(syntax), expect_valid_syntax) << syntax.message();
        if (static_cast<bool>(syntax) != expect_valid_syntax) {
            return struo::err(struo::PARSE_ERROR, "unexpected fixture syntax: " + path.string());
        }

        return struo::load<Config, typename Descriptor::Format>(path);
    }
};

} // namespace e2e
