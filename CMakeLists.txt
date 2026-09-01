cmake_minimum_required(VERSION 3.24)
project(struo VERSION 0.1 LANGUAGES CXX)

find_package(magic_enum CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

set(STRUO_DEBUG AUTO CACHE STRING "Enable struo debug mode: AUTO, ON or OFF")
set_property(CACHE STRUO_DEBUG PROPERTY STRINGS AUTO ON OFF)

option(STRUO_BUILD_TESTS "Build struo tests" ${PROJECT_IS_TOP_LEVEL})

add_library(struo_warnings INTERFACE)
add_library(struo_defs INTERFACE)
add_library(struo)
add_library(struo::struo ALIAS struo)

target_compile_definitions(struo_defs
        INTERFACE
        STRUO_VERSION="${PROJECT_VERSION}"
)

set(STRUO_DEBUG_ENABLED OFF)
if(STRUO_DEBUG STREQUAL "ON")
    set(STRUO_DEBUG_ENABLED ON)
elseif(STRUO_DEBUG STREQUAL "AUTO")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(STRUO_DEBUG_ENABLED ON)
    else()
        set(STRUO_DEBUG_ENABLED OFF)
    endif()
elseif(STRUO_DEBUG STREQUAL "OFF")
    set(STRUO_DEBUG_ENABLED OFF)
else()
    message(FATAL_ERROR "STRUO_DEBUG must be AUTO, ON, or OFF (got: '${STRUO_DEBUG}')")
endif()

if(STRUO_DEBUG_ENABLED)
    target_compile_definitions(struo_defs INTERFACE STRUO_DEBUG=1)
else()
    target_compile_definitions(struo_defs INTERFACE STRUO_DEBUG=0)
endif()

target_sources(struo
        PRIVATE
        src/struo/detail/detail.cpp)

target_include_directories(struo
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(struo
        PUBLIC
        yaml-cpp::yaml-cpp
        magic_enum::magic_enum
        stdc++exp

        PUBLIC
        struo_defs

        PRIVATE
        struo_warnings
)

target_compile_features(struo PUBLIC cxx_std_23)

if(STRUO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()