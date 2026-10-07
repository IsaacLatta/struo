# Struo tests

## Run locally

From the repository root, using the checked-in vcpkg developer preset:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --test-dir build/dev --output-on-failure
```

The unit executable is `struo_tests`. The E2E executable is `struo_e2e_tests`;
its discovered tests have the `e2e.` prefix and `e2e` CTest label.

```sh
ctest --test-dir build/dev --output-on-failure -L e2e
ctest --test-dir build/dev --output-on-failure -L e2e -R FullConfiguration
ctest --test-dir build/dev --output-on-failure -L e2e -R 'e2e\.Vision'
```

For direct GoogleTest execution, run from the project root:

```sh
./build/dev/tests/struo_tests
./build/dev/tests/struo_e2e_tests
./build/dev/tests/struo_e2e_tests --gtest_filter='Vision/*.*'
```

CMake anchors configuration fixture paths to the source tree and sets the project
root as the working directory for CTest. Relative paths *inside* a loaded configuration
are interpreted against the project root.

Unit test sources live in `unit/`, and end to end are found in `e2e/`.
Shared helpers and schemas live in `include/`, and documents/resources in `fixtures/`.

## E2E cases

`fixtures/web_server/` contains independently authored YAML, JSON and TOML versions
of nine cases. Each case runs through the public `load<Config, Format>(path)` API,
using `struo::Yaml`, `struo::Json` and `struo::Toml`.

| Case | Purpose |
| --- | --- |
| `full` | API gateway with TLS, two upstream endpoints, all action/middleware alternatives and explicit nested settings. |
| `minimal` | Required settings with omitted optionals and defaults. |
| `nested_defaults` | Explicit empty sections and omitted defaults inside upstreams and action variants. |
| `missing_required` | Listener without its required port. |
| `wrong_type` | Routes object where a sequence is required. |
| `invalid_constraint` | Zero connection timeout inside a named upstream. |
| `unknown_variant` | Unrecognized action tag. |
| `missing_variant_payload` | Recognized action tag without its `value`. |
| `malformed` | Intentionally invalid native syntax. |

## Vision cases

`fixtures/vision/` contains twelve cases, each independently authored in YAML, JSON
and TOML.

| Case | Purpose |
| --- | --- |
| `full` | GigE/process/detection, USB/classification and IP/segmentation pipelines, covering ROI/Resize/Letterbox and Periodic/Serial/HTTP triggers. |
| `minimal` | IP/classification with a Periodic trigger; endpoint defaults and omitted credentials/command server. |
| `defaults` | Separate IP and GigE pipelines sharing local node names; default bandwidth/compression/segmentation threshold and absent optional settings. |
| `video` | Replay pipeline using the Video camera's `file` alias and checked-in resource. |
| `missing_required` | Periodic trigger without its interval. |
| `wrong_type` | Sequence supplied where a node map is required. |
| `invalid_constraint` | Detection threshold outside [0, 1]. |
| `unknown_variant` | Unrecognized nested camera tag. |
| `missing_variant_payload` | Classification-model tag without its `value`. |
| `missing_reference` | Model input absent from its pipeline and every other pipeline. |
| `cross_pipeline_reference` | Target exists only in a different pipeline; the custom local-reference constraint rejects it. |
| `malformed` | Intentionally invalid native syntax. |

`fixtures/resources/video.txt` is a regular file used solely for the Video camera's
`FileExists` constraint.

## Add a format or scenario

1. Add the format's public alias and file-loading support in Struo. The library's
   `detail::FormatReader` is internal, not a supported user customization API.
2. Add a descriptor in `include/e2e/fixtures.hpp` with `Format`, `extension` and
   `validateSyntax(contents)`. The syntax check uses the native format library.
3. Register it once in `e2e::AllFormats`. Both scenario suites use this shared list.
4. Author matching documents for every applicable case in each scenario's
   fixture directory.
5. Author tests loading each of your applicable cases.


For a new scenario, derive its typed fixture from `e2e::FixtureTest<Config, Descriptor>`,
register against `e2e::AllFormats`, call `loadCase(scenario, stem)` (or pass `false`
for intentional syntax errors), place its source under `e2e/`, and add it to the E2E target. Expectation code
belongs in the scenario's tests, while syntax/readability preflight belongs in the shared test harness.
