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

For direct GoogleTest execution, use the fixture directory as the working directory:

```sh
cd tests/fixtures
../../build/dev/tests/struo_e2e_tests
../../build/dev/tests/struo_e2e_tests --gtest_filter='Vision/*.*'
```

CMake anchors configuration fixture paths to the source tree and sets this working
directory for CTest. Relative paths *inside* a loaded configuration are interpreted
against that working directory. Keep checked-in fixtures read-only.

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

Successful cases assert independently specified expected typed values, shared across
formats. Schema-invalid cases first establish readable input and valid native syntax,
then assert loading failure only. Malformed cases establish readable input and invalid
syntax. Missing files and unexpected syntax fail the test rather than satisfy a negative
loading assertion. Parse/schema error codes, paths and messages are not fixed by these tests.

The paths in the web-server fixtures are inert configuration values; no server is started.

## Vision cases

`fixtures/vision/` contains twelve cases, each independently authored in YAML, JSON
and TOML. Together, both scenarios register 63 E2E instances (27 web-server and 36 vision)
against the same `e2e::AllFormats` list and public loading API.

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

Successful fixtures declare referring nodes before their targets and reuse local names
across pipelines. Expected values are asserted independently for each pipeline, including
variant payloads, processor order, geometry, class maps, durations, defaults and optionals.
The reference constraint checks existence within each pipeline; it does not check cycles
or target types. Negative cases use the same preflight and failure-only policy as web-server tests.

`fixtures/resources/video.txt` is an inert regular file used solely for the Video camera's
`FileExists` constraint. It is not a playable video and is never decoded. Configurations
reference `resources/video.txt` relative to the fixture working directory; direct runs
therefore need the working directory shown above. Serial-device paths, endpoints and
credentials are fixture values; tests do not access hardware or start services.

## Add a format or scenario

1. Add the format's public alias and file-loading support in Struo. The library's
   `detail::FormatReader` is internal, not a supported user customization API.
2. Add a descriptor in `include/e2e/fixtures.hpp` with `Format`, `extension` and
   `validateSyntax(contents)`. The syntax check uses the native format library.
3. Register it once in `e2e::AllFormats`. Both scenario suites use this shared list.
4. Author matching native documents for every applicable case in each scenario's
   fixture directory. Do not generate documents from a shared intermediate representation.
5. Run the E2E label and full regression suite. Existing scenario assertions require
   no format-specific copies; missing fixtures fail loudly.

For a new scenario, derive its typed fixture from `e2e::FixtureTest<Config, Descriptor>`,
register against `e2e::AllFormats`, call `loadCase(scenario, stem)` (or pass `false`
for intentional syntax errors), and add its source to the E2E target. Expectation code
belongs in the scenario source; syntax/readability preflight belongs in the shared harness.
