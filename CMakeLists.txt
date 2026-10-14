cmake_minimum_required(VERSION 3.24)
project(struo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(magic_enum CONFIG REQUIRED)

option(STRUO_YAML "Enable YAML Support" OFF)
option(STRUO_JSON "Enable JSON Support" OFF)
option(STRUO_TOML "Enable TOML Support" OFF)
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

if(MSVC)
    # For __VA_OPT__ in STRUO_ASSERT, ref: https://learn.microsoft.com/en-us/cpp/build/reference/zc-preprocessor?view=msvc-170
    target_compile_options(struo_defs INTERFACE /Zc:preprocessor)

    target_compile_options(struo_warnings INTERFACE
        /W4
        /permissive-
        /w14242
        /w14254
        /w14263
        /w14265
        /w14287
        /w14296
        /w14311
        /w14826
        /w14640
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
        magic_enum::magic_enum
        struo_defs

        PRIVATE
        struo_warnings
)

if(STRUO_YAML)
    find_package(yaml-cpp CONFIG REQUIRED)
    target_link_libraries(struo PUBLIC yaml-cpp::yaml-cpp)
endif()

if(STRUO_TOML)
    find_package(tomlplusplus CONFIG REQUIRED)
    target_link_libraries(struo PUBLIC tomlplusplus::tomlplusplus)
endif()

if(STRUO_JSON)
    find_package(nlohmann_json CONFIG REQUIRED)
    target_link_libraries(struo PUBLIC nlohmann_json::nlohmann_json)
endif()

target_compile_features(struo PUBLIC cxx_std_20)

if(STRUO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
