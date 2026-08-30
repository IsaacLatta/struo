find_package(GTest CONFIG REQUIRED)

add_executable(struo_tests)

target_sources(struo_tests
        PRIVATE
        dummy_test.cpp
)

target_include_directories(struo_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(struo_tests
        PRIVATE
        struo::struo
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(struo_tests)