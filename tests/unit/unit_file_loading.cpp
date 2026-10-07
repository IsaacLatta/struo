#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <streambuf>
#include <string>

#include "struo/struo.hpp"
#include "scoped.hpp"

namespace {

// The public loader requires an object schema, even when file I/O fails
// before parsing. No configuration fields or format-specific fixtures are needed.
struct EmptyConfig {};

} // namespace

namespace struo {

template<>
struct SchemaTraits<EmptyConfig> {
    static auto schema() {
        return Object{};
    }
};

} // namespace struo

namespace {

using namespace struo;
using testlib::ScopedDirectory;
using testlib::ScopedFile;

template<typename Format>
class FileLoading : public ::testing::Test {};

using FileFormats = ::testing::Types<Yaml, Json, Toml>;
TYPED_TEST_SUITE(FileLoading, FileFormats);

TYPED_TEST(FileLoading, MissingFileReturnsFileNotFound) {
    const ScopedDirectory directory;
    const auto result = load<EmptyConfig, TypeParam>(directory.getPath() / "missing");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), FILE_NOT_FOUND);
}

TYPED_TEST(FileLoading, DirectoryReturnsOpenFileFailed) {
    const ScopedDirectory directory;
    const auto result = load<EmptyConfig, TypeParam>(directory.getPath());
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), OPEN_FILE_FAILED);
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
