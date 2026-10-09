cmake_minimum_required(VERSION 3.24)
project(struo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(magic_enum CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

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

# The following warnings are from: https://github.com/cpp-best-practices/cppbestpractices/blob/master/02-Use_the_Tools_Available.md
if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(struo_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(struo_warnings INTERFACE
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wnull-dereference
        -Wuseless-cast
        -Wsuggest-override
    )
endif()

target_sources(struo
        PRIVATE
        src/struo/detail/detail.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(struo PRIVATE src/struo/detail/platform_linux.cpp)
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
        nlohmann_json::nlohmann_json
        tomlplusplus::tomlplusplus

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
