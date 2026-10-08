find_package(GTest CONFIG REQUIRED)

add_executable(struo_tests)

target_sources(struo_tests
        PRIVATE
        unit/unit_template_constraints.cpp
        unit/unit_defaults.cpp
        unit/unit_variants.cpp
        unit/unit_optionals.cpp
        unit/unit_yaml_parser.cpp
        unit/unit_json_parser.cpp
        unit/unit_toml_parser.cpp
        unit/unit_file_loading.cpp
        unit/unit_scalar_types.cpp
        unit/unit_result.cpp
        unit/unit_asserts_on.cpp
        unit/unit_asserts_off.cpp
        unit/unit_range_constraints.cpp
        unit/unit_size_constraints.cpp
        unit/unit_platform_constraints.cpp
        unit/unit_combinator_constraints.cpp
        unit/unit_traversal.cpp
        unit/unit_value_constraints.cpp
        unit/unit_transforms.cpp
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

gtest_discover_tests(struo_tests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(struo_e2e_tests e2e/e2e_web_server.cpp e2e/e2e_vision.cpp)
target_include_directories(struo_e2e_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(struo_e2e_tests PRIVATE struo::struo GTest::gtest_main)
target_compile_definitions(struo_e2e_tests PRIVATE
    STRUO_TEST_FIXTURE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)
gtest_discover_tests(struo_e2e_tests
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    TEST_PREFIX "e2e."
    PROPERTIES LABELS "e2e"
)
