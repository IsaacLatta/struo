#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <vector>

#include "struo/struo.hpp"
#include "scoped.hpp"

namespace {

struct FileDatabase {
    std::string host;
    int port{};
};

struct FileConfig {
    std::string name;
    int workers{};
    std::optional<FileDatabase> database;
    std::vector<std::string> tags;
};

struct ThrowingFileConfig {
    std::string name;
};

} // namespace

namespace struo {

template<>
struct SchemaTraits<FileDatabase> {
    static auto schema() {
        return Object{
            Field<&FileDatabase::host>{Keys{"host"}, REQUIRED},
            Field<&FileDatabase::port>{Keys{"port"}, Defaults{Value<5432>}}
        };
    }
};

template<>
struct SchemaTraits<FileConfig> {
    static auto schema() {
        return Object{
            Field<&FileConfig::name>{Keys{"name"}, REQUIRED, Constraints{NotEmpty}},
            Field<&FileConfig::workers>{Keys{"workers"}, Defaults{Value<4>}, Constraints{AtLeast<1>}},
            Field<&FileConfig::database>{Keys{"database"}},
            Field<&FileConfig::tags>{Keys{"tags"}}
        };
    }
};

template<>
struct SchemaTraits<ThrowingFileConfig> {
    static auto schema() {
        return Object{
            Field<&ThrowingFileConfig::name>{Keys{"name"}, REQUIRED, Constraints{
                [](const std::string&) -> Result<void> {
                    throw std::runtime_error{"user constraint failed"};
                }
            }}
        };
    }
};

} // namespace struo

namespace {

using namespace struo;
using testlib::ScopedDirectory;
using testlib::ScopedFile;

struct YamlFiles {
    using Format = Yaml;
    static constexpr std::string_view full = R"(
name: owned configuration
workers: 7
database: {host: database.local, port: 6000}
tags: [primary, backup]
)";
    static constexpr std::string_view minimal = "name: minimal\n";
    static constexpr std::string_view malformed = "name: [unterminated";
    static constexpr std::string_view missing = "workers: 2\n";
    static constexpr std::string_view wrong_type = "name: []\n";
    static constexpr std::string_view invalid = "name: invalid\nworkers: 0\n";
};

struct JsonFiles {
    using Format = Json;
    static constexpr std::string_view full = R"({
        "name": "owned configuration", "workers": 7,
        "database": {"host": "database.local", "port": 6000},
        "tags": ["primary", "backup"]
    })";
    static constexpr std::string_view minimal = R"({"name": "minimal"})";
    static constexpr std::string_view malformed = R"({"name":)";
    static constexpr std::string_view missing = R"({"workers": 2})";
    static constexpr std::string_view wrong_type = R"({"name": []})";
    static constexpr std::string_view invalid = R"({"name": "invalid", "workers": 0})";
};

struct TomlFiles {
    using Format = Toml;
    static constexpr std::string_view full = R"(
name = "owned configuration"
workers = 7
tags = ["primary", "backup"]
[database]
host = "database.local"
port = 6000
)";
    static constexpr std::string_view minimal = R"(name = "minimal")";
    static constexpr std::string_view malformed = "name = [";
    static constexpr std::string_view missing = "workers = 2";
    static constexpr std::string_view wrong_type = "name = []";
    static constexpr std::string_view invalid = "name = \"invalid\"\nworkers = 0\n";
};

template<typename T>
class FileLoading : public ::testing::Test {};

using FileFormats = ::testing::Types<YamlFiles, JsonFiles, TomlFiles>;
TYPED_TEST_SUITE(FileLoading, FileFormats);

template<typename Format>
concept CanLoadFile = requires(const std::filesystem::path& path) {
    { load<FileConfig, Format>(path) } -> std::same_as<Result<FileConfig>>;
};

struct UnsupportedFormat {};
static_assert(CanLoadFile<Yaml> && CanLoadFile<Json> && CanLoadFile<Toml>);
static_assert(!CanLoadFile<UnsupportedFormat>);

TYPED_TEST(FileLoading, LoadsOwningValuesIndependentOfFileExtension) {
    using Format = typename TypeParam::Format;
    std::filesystem::path path;
    const auto result = [&] {
        const ScopedFile file{TypeParam::full, "unrelated"};
        path = file.getPath();
        return load<FileConfig, Format>(path);
    }();

    ASSERT_FALSE(std::filesystem::exists(path));
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().name, "owned configuration");
    EXPECT_EQ(result.value().workers, 7);
    ASSERT_TRUE(result.value().database);
    EXPECT_EQ(result.value().database->host, "database.local");
    EXPECT_EQ(result.value().database->port, 6000);
    EXPECT_EQ(result.value().tags, (std::vector<std::string>{"primary", "backup"}));
}

TYPED_TEST(FileLoading, AppliesDefaultsAndOmittedOptionals) {
    const ScopedFile file{TypeParam::minimal, "data"};
    const auto result = load<FileConfig, typename TypeParam::Format>(file.getPath());
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().name, "minimal");
    EXPECT_EQ(result.value().workers, 4);
    EXPECT_FALSE(result.value().database);
    EXPECT_TRUE(result.value().tags.empty());
}

TYPED_TEST(FileLoading, MissingFileReturnsFileNotFound) {
    const ScopedDirectory directory;
    const auto result = load<FileConfig, typename TypeParam::Format>(directory.getPath() / "missing");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), FILE_NOT_FOUND);
}

TYPED_TEST(FileLoading, DirectoryReturnsOpenFileFailed) {
    const ScopedDirectory directory;
    const auto result = load<FileConfig, typename TypeParam::Format>(directory.getPath());
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), OPEN_FILE_FAILED);
}

TYPED_TEST(FileLoading, MalformedDocumentReturnsFailure) {
    const ScopedFile file{TypeParam::malformed, "data"};
    const auto result = load<FileConfig, typename TypeParam::Format>(file.getPath());
    EXPECT_FALSE(result);
}

TYPED_TEST(FileLoading, ValidSyntaxWithInvalidSchemaReturnsFailure) {
    using Format = typename TypeParam::Format;
    for (const auto contents : {TypeParam::missing, TypeParam::wrong_type, TypeParam::invalid}) {
        SCOPED_TRACE(std::string{contents});
        // Ensure a failure below cannot be satisfied by malformed test input.
        ASSERT_TRUE(detail::FormatReader<Format>::parse(contents));
        const ScopedFile file{contents, "data"};
        const auto result = load<FileConfig, Format>(file.getPath());
        EXPECT_FALSE(result);
    }
}

TYPED_TEST(FileLoading, DoesNotReclassifyUserConstraintExceptions) {
    const ScopedFile file{TypeParam::minimal, "data"};
    EXPECT_THROW((void(load<ThrowingFileConfig, typename TypeParam::Format>(file.getPath()))), std::runtime_error);
}

TYPED_TEST(FileLoading, ExistingParserInstanceOverloadStillWorks) {
    using Format = typename TypeParam::Format;
    auto document = detail::FormatReader<Format>::parse(TypeParam::full);
    ASSERT_TRUE(document);
    auto parser = detail::FormatReader<Format>::makeParser(*document);
    const auto result = load<FileConfig>(parser);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().name, "owned configuration");
}

TYPED_TEST(FileLoading, ResolvesRelativePathsFromWorkingDirectory) {
    const ScopedFile file{TypeParam::minimal, "data"};
    const auto relative = std::filesystem::relative(file.getPath(), std::filesystem::current_path());
    ASSERT_TRUE(relative.is_relative());
    const auto result = load<FileConfig, typename TypeParam::Format>(relative);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().name, "minimal");
}

TEST(FileReading, PreservesEmptyPartialExactAndMultipleBuffers) {
    for (const std::size_t size : {0u, 31u, 8192u, 20001u}) {
        SCOPED_TRACE(size);
        std::string expected(size, 'x');
        if (size > 10) expected[10] = '\0';
        std::istringstream input{expected};
        const auto result = detail::read_stream(input);
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, expected);
    }
}

class FailingBuffer : public std::streambuf {
    std::streamsize xsgetn(char*, std::streamsize) override {
        throw std::ios_base::failure{"injected read failure"};
    }
};

TEST(FileReading, ReportsReadFailureWithAndWithoutStreamExceptions) {
    for (const bool exceptions : {false, true}) {
        SCOPED_TRACE(exceptions);
        FailingBuffer buffer;
        std::istream input{&buffer};
        if (exceptions) input.exceptions(std::ios::badbit);
        const auto result = detail::read_stream(input);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().code(), READ_FILE_FAILED);
    }
}

TEST(FileReading, RejectsAnAlreadyFailedStream) {
    std::istringstream input{"contents"};
    input.setstate(std::ios::failbit);
    const auto result = detail::read_stream(input);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), READ_FILE_FAILED);
}

TEST(FileReading, AcceptsNormalEofWithStreamExceptionsEnabled) {
    std::istringstream input{"partial buffer"};
    input.exceptions(std::ios::failbit | std::ios::badbit);
    const auto result = detail::read_stream(input);
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, "partial buffer");
}

TEST(FileReading, ReadsActualFilesAcrossBufferBoundaries) {
    const std::string expected(20001, 'x');
    const ScopedFile file{expected, "data"};
    const auto result = detail::read_file(file.getPath());
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, expected);
}

} // namespace
