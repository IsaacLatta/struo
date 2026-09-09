cmake_minimum_required(VERSION 3.24)
project(struo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(magic_enum CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

option(STRUO_BUILD_TESTS "Build struo tests" ${PROJECT_IS_TOP_LEVEL})

add_library(struo_warnings INTERFACE)
add_library(struo_defs INTERFACE)
add_library(struo)
add_library(struo::struo ALIAS struo)

target_compile_definitions(struo_defs
        INTERFACE
        STRUO_VERSION="${PROJECT_VERSION}"
        STRUO_PLATFORM_LINUX=$<PLATFORM_ID:Linux>
)

target_sources(struo
        PRIVATE
        src/struo/detail/detail.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
target_sources(struo
        PRIVATE
        src/struo/detail/platform_linux.cpp)
endif()

target_include_directories(struo
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(struo
        PUBLIC
        yaml-cpp::yaml-cpp
        magic_enum::magic_enum

        PUBLIC
        struo_defs

        PRIVATE
        struo_warnings
)

target_compile_features(struo PUBLIC cxx_std_20)

if(STRUO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
