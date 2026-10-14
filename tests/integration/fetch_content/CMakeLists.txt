cmake_minimum_required(VERSION 3.24)
project(struo_consumer LANGUAGES CXX)

include(FetchContent)

set(STRUO_YAML ON CACHE BOOL "" FORCE)
set(STRUO_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    struo
    GIT_REPOSITORY https://github.com/IsaacLatta/struo.git
    GIT_TAG main
)

FetchContent_MakeAvailable(struo)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE struo::struo)

enable_testing()
add_test(NAME consumer COMMAND consumer)