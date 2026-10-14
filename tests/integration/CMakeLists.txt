add_test(
    NAME integration.fetch_content
    COMMAND "${CMAKE_CTEST_COMMAND}"
        --build-and-test
            "${CMAKE_CURRENT_SOURCE_DIR}/fetch_content"
            "${CMAKE_CURRENT_BINARY_DIR}/fetch_content"
        --build-generator "${CMAKE_GENERATOR}"
        --build-noclean
        --build-options
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
            "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
            "-DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}"
            "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}"
            "-DFETCHCONTENT_SOURCE_DIR_STRUO=${PROJECT_SOURCE_DIR}"
        --test-command "${CMAKE_CTEST_COMMAND}" --output-on-failure
)

set_tests_properties(integration.fetch_content PROPERTIES
    LABELS integration
)

add_test(
    NAME integration.add_subdirectory
    COMMAND "${CMAKE_CTEST_COMMAND}"
        --build-and-test
            "${CMAKE_CURRENT_SOURCE_DIR}/add_subdirectory"
            "${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory"
        --build-generator "${CMAKE_GENERATOR}"
        --build-noclean
        --build-options
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
            "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
            "-DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}"
            "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}"
            "-DSTRUO_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
        --test-command "${CMAKE_CTEST_COMMAND}" --output-on-failure
)

set_tests_properties(integration.add_subdirectory PROPERTIES
    LABELS integration
)
