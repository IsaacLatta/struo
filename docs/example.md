# Worked example: a configurable service backend

- [User types](#user-types)
- [Constraints](#constraints)
- [Transforms](#transforms)
- [Defaults](#defaults)
- [Schemas](#schemas)
- [Variants](#variants)
- [Loading](#loading)

This extends the README's service example with application-specific validation,
route normalization, and an environment-backed default. Put the C++ definitions below
in order in one `main.cpp`, skipping the standalone declaration illustrations.
Enable YAML and link Struo as described in the
[build guide](build.md).

## User types

The structs contain only application data. Their loading schemas are defined
separately below.

```cpp
#include <struo/struo.hpp>
#include <struo/yaml.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct Local { 
    std::filesystem::path directory;
};

struct Upstream {
    std::vector<std::string> hosts;
    std::optional<std::string> health_check;
    std::string route;
    int port{};
};

enum class BackendKind { 
    LOCAL, 
    UPSTREAM 
};

using Backend = std::variant<Local, Upstream>;

struct Config { 
    Backend backend; 
};
```

## Constraints

A constraint is a callable that receives a field's value and returns `struo::Result<void>`. Lambdas and function objects both
work. As an example, we define our own constraint that routes must be paths without a query string:

```cpp
struo::Result<void> routeWithoutQuery(const std::string& route) {
    if (route.find('?') != std::string::npos) {
        return struo::err(struo::INVALID_VALUE, "route must not contain a query string");
    }
    return struo::ok();
}
```

Now add it to the field's `Constraints` pack:

```cpp
struo::Constraints{routeWithoutQuery}
```

This is the pack passed to the route field in the [schema below](#schemas).

The schema also uses `Or<IsValidIpv4, IsValidHostname>` to accept either an IPv4
address or a hostname. `ForEach` checks each host in the vector, while `NotEmpty`
first requires at least one host. Constraints run in their listed order and stop
at the first failure:

```cpp
Constraints{NotEmpty, ForEach<Or<IsValidIpv4, IsValidHostname>>}
```

For an `std::optional` health-check route, `IfPresent` runs constraints only when
a value is present:

```cpp
Constraints{IfPresent<routeWithoutQuery>}
```

## Transforms

Like constraints, a transform is also a callable, and receives the current value and
returns its replacement. Return `T` for a simple transformation, or `Result<T>` when it can fail. 
This custom transform removes trailing slashes while preserving the root route `/`:

```cpp
std::string removeTrailingSlashes(const std::string& route) {
    auto result = route;
    while (result.size() > 1 && result.back() == '/') {
        result.pop_back();
    }
    return result;
}
```

For example, `" api/// "` becomes `/api`. Transforms run in their listed order,
before constraints, for both supplied values and schema defaults.

```cpp
Transforms{TrimWhitespace, AddLeadingSlash, removeTrailingSlashes}
```

The same ordering applies inside adapters: trim every host with `ForEach`, or
trim and prefix the health-check route only when it is present with `IfPresent`.

```cpp
Transforms{ForEach<TrimWhitespace>}
Transforms{IfPresent<TrimWhitespace, AddLeadingSlash>}
```

## Defaults

Defaults similarily are callables that supply missing fields and take no arguments. 
`FromEnv<"APP_UPSTREAM_PORT">` reads the port from an environment variable, while a custom lambda supplies `/` 
as the default route:

```cpp
inline constexpr auto defaultRoute = [] { return std::string{"/"}; };
```

Pass that provider to the route field's defaults pack:

```cpp
Defaults{defaultRoute}
```

For a provider that can decline or fail, return `Result<std::optional<T>>`:

- a present `std::optional` supplies a value
- an empty `std::optional` tries the next provider
- and an error stops loading.

For example, this first provider declines, so the next supplies the route:

```cpp
Defaults{[] { return std::optional<std::string>{}; }, defaultRoute}
```

A configured port takes precedence over `FromEnv`. If the port is omitted, an
unset or invalid `APP_UPSTREAM_PORT` causes an error. Defaults only apply to
missing fields. They do not replace invalid configured values.

```cpp
Field<&Upstream::port>{Keys{"port"}, Defaults{FromEnv<"APP_UPSTREAM_PORT">},
    Constraints{Range<1, 65535>}}
```

## Schemas

Specialize `SchemaTraits<T>` for each object. `Keys` name the _key_ the input field should be parsed under. Multiple
keys provide aliases, checked in order. Here `directory` also accepts `root`.
Its `std::filesystem::path` member accepts a path from the configuration directly;
`TrimWhitespace` and `NotEmpty` work on it too.

```cpp
namespace struo {
template<> struct SchemaTraits<Local> {
    static auto schema() {
        return Object{
            Field<&Local::directory>{Keys{"directory", "root"},
                Transforms{TrimWhitespace}, Constraints{NotEmpty}}
        };
    }
};

template<> struct SchemaTraits<Upstream> {
    static auto schema() {
        return Object{
            
            Field<&Upstream::hosts>{Keys{"hosts"},
                Transforms{ForEach<TrimWhitespace>},
                Constraints{NotEmpty, ForEach<Or<IsValidIpv4, IsValidHostname>>}},

            Field<&Upstream::port>{Keys{"port"},
                Defaults{FromEnv<"APP_UPSTREAM_PORT">},
                Constraints{Range<1, 65535>}},
            
            Field<&Upstream::route>{Keys{"route"},
                Defaults{defaultRoute},
                Transforms{TrimWhitespace, AddLeadingSlash, removeTrailingSlashes},
                Constraints{routeWithoutQuery}},
            
            Field<&Upstream::health_check>{Keys{"health_check"}, OPTIONAL,
                Transforms{IfPresent<TrimWhitespace, AddLeadingSlash>},
                Constraints{IfPresent<routeWithoutQuery>}}
        };
    }
};

}
```

Fields are required by default, but a schema default can satisfy that requirement.
An `OPTIONAL` field with no supplied value or default keeps its member initializer
and skips transforms and constraints. Fields are required by default, except `std::optional<T>`
fields, which default to `OPTIONAL`. A `Presence` argument in the field's constructor overrides these defaults.
Constraints on an `std::optional<T>` receive the optional itself, but the `IfPresent` wrapper 
can apply constraints on the optional's type when present.

```cpp
Field<&Upstream::health_check>{Keys{"health_check"}, OPTIONAL,
    Constraints{IfPresent<routeWithoutQuery>}}
```

## Variants

`std::variant` is supported through binding each alternative to a tag. Enum tags use their enumerator
names and are case-sensitive. The default keys are `type` and `value`, but the
`TagKey` and `ContentKey` can customize them.

```cpp
namespace struo {
template<> struct SchemaTraits<Backend> {
    static auto schema() {
        return Variant{Bindings{
            Bind<BackendKind::LOCAL, Local>{},
            Bind<BackendKind::UPSTREAM, Upstream>{}
        }};
    }
};

template<> struct SchemaTraits<Config> {
    static auto schema() {
        return Object{Field<&Config::backend>{Keys{"backend"}}};
    }
};
} 
```

An unknown tag or a missing `value` is an error. The selected alternative's own
schema supplies its defaults, transforms, and constraints.

## Loading

Save this as `config.yaml`:

```yaml
backend:
  type: UPSTREAM
  value:
    hosts: [" example.com ", "192.0.2.10"]
    route: " api/// "
    health_check: health
```

Set the port default before running (or add `port: 8080` to the YAML):

```sh
export APP_UPSTREAM_PORT=8080
```

Finish `main.cpp` with:

```cpp
int main() {
    auto config = struo::load<Config, struo::Yaml>("config.yaml");
    if (!config) {
        std::cerr << config.error().what() << '\n';
        return 1;
    }

    if (const auto* upstream = std::get_if<Upstream>(&(*config).backend)) {
        for (const auto& host : upstream->hosts) {
            std::cout << host << ':' << upstream->port << upstream->route << '\n';
        }
    }
}
```

With `APP_UPSTREAM_PORT=8080`, this prints:

```text
example.com:8080/api
192.0.2.10:8080/api
```

The optional health-check route becomes `/health`. Omit `health_check` to leave
it empty,  omit `route` to use the custom `/` default, and add `port: 0` to see a
range failure. Set `hosts: ["bad host"]` to exercise host validation, or set
`route: "api?debug=true"` to exercise the custom constraint.

To select the other alternative, replace the YAML with:

```yaml
backend:
  type: LOCAL
  value:
    directory: ./public
```

The format is selected explicitly by `load`, not inferred from the file extension.
Relative file paths use the process's working directory, paths inside the
configuration are not automatically made relative to the configuration file.

For larger configurations, see the [web-server](../tests/include/schemas/web_server.hpp)
and [computer vision](../tests/include/schemas/cv.hpp) schemas in the tests.
