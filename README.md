# struo

Struo is a C++20 configuration library for loading YAML, JSON, and TOML into
your own types. Define a schema that describes fields, variants, defaults,
transforms, and constraints; Struo handles loading and populating your structs.

## Why

Complex configurations change as applications grow. Struo aims to make them easy
to represent and quick to iterate on, removing the need to write application
parsing code. Validation, defaulting, and populating user structs belong alongside
the schema, so adding a configuration option does not require another handwritten
parsing routine.

## Design goals

- **Represent complex configurations:** compose objects, containers, and tagged
  variants to describe the choices your application supports.
- **Iterate quickly:** keep field names, defaults, transforms, and constraints
  together in an explicit loading schema.
- **Keep user types independent:** write ordinary structs and define their loading
  schemas separately, without inheritance, macros, or annotations on the types.
- **Make post-parsing work easy:** normalize and validate values before receiving
  the populated configuration, with errors identifying the affected value.

## A small example

A service can serve a local directory or forward requests to an upstream. The
schema validates each upstream host with `ForEach`, constrains ports, and
normalizes the upstream route with `AddLeadingSlash`.

```cpp
#include <struo/struo.hpp>
#include <struo/yaml.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

struct Local { 
    std::filesystem::path directory;
};

struct Upstream {
    std::vector<std::string> hosts;
    int port{};
    std::string route;
};

enum class BackendKind { 
    LOCAL, 
    UPSTREAM 
};

using Backend = std::variant<Local, Upstream>;

struct Config { 
    Backend backend; 
};

namespace struo {
template<> 
struct SchemaTraits<Local> {
    static auto schema() {
        return Object{
            Field<&Local::directory>{Keys{"directory"}, Constraints{NotEmpty}}
        };
    }
};

template<> 
struct SchemaTraits<Upstream> {
    static auto schema() {
        return Object{
            Field<&Upstream::hosts>{Keys{"hosts"},
                Transforms{ForEach<TrimWhitespace>},
                Constraints{NotEmpty, ForEach<Or<IsValidIpv4, IsValidHostname>>}},
            Field<&Upstream::port>{Keys{"port"}, Defaults{Value<8080>}, Constraints{Range<1, 65535>}},
            Field<&Upstream::route>{Keys{"route"}, Transforms{TrimWhitespace, AddLeadingSlash}}
        };
    }
};

template<> 
struct SchemaTraits<Backend> {
    static auto schema() {
        return Variant{Bindings{
            Bind<BackendKind::LOCAL, Local>{},
            Bind<BackendKind::UPSTREAM, Upstream>{}
        }};
    }
};

template<> 
struct SchemaTraits<Config> {
    static auto schema() {
        return Object{Field<&Config::backend>{Keys{"backend"}}};
    }
};

}  

int main() {
    auto config = struo::load<Config, struo::Yaml>("config.yaml");
    if (!config) {
        // Give users a diagnostic identifying the configuration value to fix.
        std::cerr << config.error().what() << '\n';
        return 1;
    }
    return run_app(*config); // use in your application
}
```

With this `config.yaml`, the upstream receives port `8080` and route `/api`:

```yaml
backend:
  type: UPSTREAM
  value:
    hosts: [example.com, 192.0.2.10]
    route: api
```

Give your users useful errors without constructing the configuration path yourself.
Adding `port: 0` under `value` makes the error branch above print:

```text
backend<UPSTREAM>.value.port: "range" constraint failed: expected between 1 and 65535 inclusive, got 0
```

The path identifies the `backend` field, its selected `UPSTREAM` alternative, and
the invalid `port` inside `value`. Nested diagnostics also track sequence indices
and map keys.

## Documentation

- [Building and linking](docs/build.md): dependencies, CMake options, and compiler coverage.
- [Worked example](docs/example.md): variants and custom constraints, transforms, and defaults.
- [Tests](tests/README.md): running the test suite and exploring larger configurations.
