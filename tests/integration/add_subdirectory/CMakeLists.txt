cmake_minimum_required(VERSION 3.24)
project(struo_consumer LANGUAGES CXX)

set(STRUO_YAML ON CACHE BOOL "" FORCE)
set(STRUO_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory("${STRUO_SOURCE_DIR}" struo)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE struo::struo)

enable_testing()
add_test(NAME consumer COMMAND consumer)
