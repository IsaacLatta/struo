#pragma once

#include <expected>
#include <string>
#include <filesystem>

#include "parsing.hpp"
#include "struo/Error.hpp"

#include "struo/Object.hpp"
#include "struo/concepts.hpp"

#include "struo/detail/detail.hpp"

namespace struo {

    class YamlParser {
    public:
        // very minimal interface
        // users can plug in their own parsers.
        // Yaml can be extended (e.g., overloads for chrono or what not),
        // or maybe the function traversing converts say enums -> strings
        // and chronos -> uints, etc.

        // scalar
        template<typename T>
        [[nodiscard]] constexpr std::optional<T> getAs(std::string_view);

        // child object
        [[nodiscard]] constexpr std::optional<YamlParser> toChild(std::string_view);

        // sequence / map? Maybe make separate methods for each? Then a json
        // parser can convert ints to strings when accessed as keys (as an example)?
        [[nodiscard]] constexpr std::optional<auto> getMembers();
    };

    template<typename T, typename Schema, typename Parser>
    [[nodiscard]] constexpr std::expected<T, Error> parse(const std::string& contents, Parser&& parser, T& t) {

    }

    template<typename T, typename Schema, typename Parser>
    [[nodiscard]] constexpr std::expected<T, Error> parse(const std::string& contents, Parser&& parser) {
        auto schema_object = SchemaTraits<Schema>::schema();

        return schema_object.forEachField([&](auto& field) {
            using field_type = decltype(field);
            using value_type = typename field_type::value_type;

            if constexpr (IsScalar<value_type>) {

            } else if constexpr (IsSequence<value_type>) {

            } else if constexpr (IsMap<value_type>) {

            } else if constexpr (IsObject<value_type>) {

            } else {
                // cleanup later
                static_assert(!sizeof(T),  "unknown Field<T>::value_type");
            }
        });
    }

    template<typename T, typename Schema>
    [[nodiscard]] constexpr std::expected<T, Error> parse(const std::string& contents, FileFormat format) {
        switch (format) {
            case FileFormat::YAML:
                return parse<T, Schema>(contents, YamlParser{});
        }
    }

    template<typename T, typename Schema>
    [[nodiscard]] constexpr std::expected<T, Error> load(const std::filesystem::path& path) {
        return detail::file_format_from_path(path)
            .and_then([&](FileFormat format) {
               return detail::file_to_string(path)
                    .and_then([&](const std::string& contents) {
                        return parse<T, Schema>(contents, format);
                    });
            });
    }

}
