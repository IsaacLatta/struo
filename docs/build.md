# Building and linking Struo

- [Dependencies](#dependencies)
- [Linking](#linking)
- [Options](#options)
- [Errors and assertions](#errors-and-assertions)
- [Compiler coverage](#compiler-coverage)
- [Building for development](#building-for-development)

## Dependencies

Struo requires CMake 3.24 or newer and a C++20 compiler and standard library with
`std::format` support. It includes compiled sources, so consumers must link the
library as well as include its headers.

| Dependency | Required for |
| --- | --- |
| `magic_enum` | All builds |
| `yaml-cpp` | YAML support |
| `nlohmann_json` | JSON support |
| `tomlplusplus` | TOML support |
| GoogleTest | Struo's tests |

Dependencies must be available to CMake's `find_package`, for example through a
vcpkg toolchain or `CMAKE_PREFIX_PATH`. Fetching Struo does not itself fetch these
dependencies. The repository's [vcpkg manifest](../vcpkg.json) includes all formats
and the test dependency for development.

## Linking

For a checkout at `external/struo`:

```cmake
cmake_minimum_required(VERSION 3.24)
project(my_app LANGUAGES CXX)

set(STRUO_YAML ON CACHE BOOL "Enable YAML support")
add_subdirectory(external/struo)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE struo::struo)
```

Alternatively, replace `add_subdirectory` with:

```cmake
include(FetchContent)
FetchContent_Declare(
    struo
    GIT_REPOSITORY https://github.com/IsaacLatta/struo.git
    GIT_TAG main
)
FetchContent_MakeAvailable(struo)
```

Set Struo's options before adding or fetching it. Currently, only `FetchContent` and `add_subdirectory`
are supported and tested, `find_package(struo)` package is not _yet_ provided.

Include `<struo/struo.hpp>` for the schema API and the header for each format used:

| Format | Header | Loading selector |
| --- | --- | --- |
| YAML | `<struo/yaml.hpp>` | `struo::Yaml` |
| JSON | `<struo/json.hpp>` | `struo::Json` |
| TOML | `<struo/toml.hpp>` | `struo::Toml` |

## Options

| CMake option | Default | Effect |
| --- | --- | --- |
| `STRUO_YAML` | `OFF` | Find and link yaml-cpp |
| `STRUO_JSON` | `OFF` | Find and link nlohmann_json |
| `STRUO_TOML` | `OFF` | Find and link tomlplusplus |
| `STRUO_BUILD_TESTS` | `ON` when built as the top-level project; otherwise `OFF` | Build Struo's tests |

Linking `struo::struo` propagates its C++20 requirement, include paths, and enabled
format dependencies. On MSVC it also enables `/Zc:preprocessor`. Use normal CMake 
settings such as `CMAKE_CXX_COMPILER` and `CMAKE_BUILD_TYPE` to select your compiler 
and build configuration. 

## Errors and assertions

Assertions check preconditions and internal invariants, preventing undefined behavior (UB), 
including within the struo implementation itself. For example, accessing the value (e.g., `T`) of a 
failed `struo::Result<T>` triggers an assertion. A failed assertion prints to `stderr` and aborts.  Assertions 
are enabled by default, including in Release builds, independently of
`NDEBUG`. Define the `STRUO_NO_ASSERT` preprocessor macro to disable them. After
adding or fetching Struo, apply it to the library and propagate it to consumers:

```cmake
target_compile_definitions(struo PUBLIC STRUO_NO_ASSERT)
```

This is a compile definition, not a CMake cache option. 

## Compiler coverage

Currently, the following platforms are built and tested on:

| Platform | Compiler | Configurations |
| --- | --- | --- |
| `ubuntu-24.04`, `ubuntu-26.04` | GCC (`g++`) | Debug, Release |
| `ubuntu-24.04`, `ubuntu-26.04` | Clang (`clang++`) | Debug, Release |
| `windows-2025` | MSVC (`cl`) | Debug, Release |
| `macos-26`, `macos-26-intel` | Apple Clang (`clang++`) | Debug, Release |

Note that compiler versions are not _yet_ pinned, so this table describes configured CI coverage rather than minimum
supported versions or the outcome of a particular run.

## Building for development

To build Struo and explore its tests, install Git, CMake 3.24+, Ninja, and a
compatible C++ compiler. Run the following commands from the repository root.

First initialize the bundled vcpkg checkout:

```sh
git submodule update --init --recursive
```

Bootstrap vcpkg on Linux or macOS:

```sh
./external/vcpkg/bootstrap-vcpkg.sh
```

On Windows, use a Visual Studio developer shell with `cl` and Ninja available:

```powershell
.\external\vcpkg\bootstrap-vcpkg.bat
```

Choose one of the checked-in presets:

| Platform/toolchain | Debug preset | Release preset |
| --- | --- | --- |
| Linux / GCC | `gcc-debug` (or `dev`) | `gcc-release` |
| Linux or macOS / Clang | `clang-debug` | `clang-release` |
| Windows / MSVC | `msvc-debug` | `msvc-release` |

Use `cmake --list-presets` to list the configure presets available on your host. For example, configure, build, and test a Linux GCC Debug build with:

```sh
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug
```

Replace `gcc-debug` in all three commands with your chosen preset. Configuration
uses the vcpkg manifest to install dependencies, including GoogleTest. These
presets enable all three formats and the tests, and treat compiler warnings as
errors. Build files go under `build/<preset>`; subsequent edits usually need only
the build and test commands.

CTest runs the unit, end-to-end, and CMake integration tests and prints output
for failures. To list tests or run only the end-to-end suite:

```sh
ctest --preset gcc-debug -N
ctest --preset gcc-debug -L e2e
```

See the [test guide](../tests/README.md) for more filters, fixtures, and example
configurations.
