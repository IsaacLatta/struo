#include <gtest/gtest.h>
#include "struo/struo.hpp"

TEST(DummyTests, Suceeds) {
    EXPECT_EQ(struo::get_project_version(), std::string_view{"0.1"});
    EXPECT_TRUE(struo::is_debug_enabled());
    EXPECT_TRUE(struo::is_yaml_linked());
    EXPECT_EQ(struo::get_enum_name(struo::FileFormat::YAML), std::string_view{"YAML"});
    SUCCEED();
}