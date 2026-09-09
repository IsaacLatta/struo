#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "struo/struo.hpp"
#include "scoped.hpp"

using namespace struo;
using namespace struo::testlib;

namespace {

TEST(FileExistsConstraint, AcceptsExistingFile) {
    ScopedFile file{"hello", ".txt"};

    auto result = FileExists(file.getPath());

    EXPECT_TRUE(result);
}

TEST(FileExistsConstraint, RejectsMissingFile) {
    ScopedDirectory directory;
    const auto missing = directory.getPath() / "does_not_exist.txt";

    auto result = FileExists(missing);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), FILE_NOT_FOUND);
}

TEST(FileExistsConstraint, AcceptsPathConvertibleTypes) {
    ScopedFile file{"hello", ".txt"};
    const std::string path = file.getPath().string();

    EXPECT_TRUE(FileExists(path));
}

TEST(DirectoryExistsConstraint, AcceptsExistingDirectory) {
    ScopedDirectory directory;

    auto result = DirectoryExists(directory.getPath());

    EXPECT_TRUE(result);
}

TEST(DirectoryExistsConstraint, RejectsExistingFile) {
    ScopedFile file{"hello", ".txt"};

    auto result = DirectoryExists(file.getPath());

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), INVALID_ARGUMENT);
}

TEST(DirectoryExistsConstraint, RejectsMissingDirectory) {
    ScopedDirectory directory;
    const auto missing = directory.getPath() / "does_not_exist";

    auto result = DirectoryExists(missing);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), FILE_NOT_FOUND);
}

TEST(PortConstraint, AcceptsValidPorts) {
    EXPECT_TRUE(IsValidPort(0));
    EXPECT_TRUE(IsValidPort(80));
    EXPECT_TRUE(IsValidPort(65535));
}

TEST(PortConstraint, RejectsInvalidPorts) {
    EXPECT_FALSE(IsValidPort(-1));
    EXPECT_FALSE(IsValidPort(65536));
}

#if STRUO_PLATFORM_LINUX

TEST(HostnameConstraint, AcceptsAndRejectsHostnames) {
    EXPECT_TRUE(IsValidHostname(std::string{"localhost"}));
    EXPECT_FALSE(IsValidHostname(std::string{"not a valid hostname"}));
}

TEST(IpConstraints, AcceptsValidAddresses) {
    EXPECT_TRUE(IsValidIpv4(std::string{"127.0.0.1"}));
    EXPECT_TRUE(IsValidIpv4(std::string{"0.0.0.0"}));
    EXPECT_TRUE(IsValidIpv4(std::string{"255.255.255.255"}));

    EXPECT_TRUE(IsValidIpv6(std::string{"::"}));
    EXPECT_TRUE(IsValidIpv6(std::string{"::1"}));
    EXPECT_TRUE(IsValidIpv6(std::string{"2001:db8::1"}));
}

TEST(IpConstraints, RejectsInvalidAddresses) {
    EXPECT_FALSE(IsValidIpv4(std::string{"256.0.0.1"}));
    EXPECT_FALSE(IsValidIpv4(std::string{"127.0.0"}));
    EXPECT_FALSE(IsValidIpv4(std::string{"localhost"}));

    EXPECT_FALSE(IsValidIpv6(std::string{"2001:db8::g"}));
    EXPECT_FALSE(IsValidIpv6(std::string{"2001::db8::1"}));
    EXPECT_FALSE(IsValidIpv6(std::string{"127.0.0.1"}));
}

#endif

} // namespace
