#pragma once

#include <expected>
#include <string>
#include <filesystem>

#include "parsing.hpp"
#include "struo/Error.hpp"

#include "struo/Object.hpp"
#include "struo/concepts.hpp"
#include "struo/definitions.hpp"

#include "struo/detail/detail.hpp"
#include "struo/detail/traits.hpp"

namespace struo {

    class YamlParser {
    public:
        // very minimal interface
        // users can plug in their own parsers.
        // Yaml can be extended (e.g., overloads for chrono or what not),
        // or maybe the function traversing converts say enums -> strings
        // and chronos -> uints, etc.

        explicit constexpr YamlParser(std::string_view contents) : contents_(contents) {}

        // scalar
        template<typename T>
        [[nodiscard]] constexpr std::expected<T, Error> getAs() const {

        }

        // child object
        [[nodiscard]] constexpr std::expected<std::optional<YamlParser>, Error> toChild(std::string_view) const {

        }

        // sequence / map? Maybe make separate methods for each? Then a json
        // parser can convert ints to strings when accessed as keys (as an example)?
        [[nodiscard]] constexpr auto getMembers() {

        }

        [[nodiscard]] constexpr auto getElements() {

        }

    private:
        std::string_view contents_;

    };

    template<typename T>
    using ParseResult = std::expected<typename detail::ValueTraits<T>::staged_type, Error>;

    template<typename T, typename Parser>
    ParseResult<T> parse_value(Parser&&);

    template<typename Field, typename Parser>
    std::expected<void, Error> parse_field(Field& field, Parser& parser) {
        using field_type = std::remove_cvref_t<decltype(field)>;
        using value_type = typename field_type::value_type;

        auto child = parser.toChild(field.getKey().value);
        if (!child) {
            return std::unexpected{child.error()};
        }

        if (!*child) {
            return {};
        }

        auto value = parse_value<value_type>(**child);
        if (!value) {
            return std::unexpected{value.error()};
        }
        field.setStagedValue(std::move(*value));

        return {};
    }

    template<typename Object, typename Parser>
    [[nodiscard]] constexpr std::expected<void, Error> parse_object(Object& object, Parser& parser) {
        return object.forEachField([&](auto& field) -> std::expected<void, Error> {
            return parse_field(field, parser);
        });
    }

    template<typename Sequence, typename Parser>
    [[nodiscard]] constexpr ParseResult<Sequence> parse_sequence(Parser& parser) {
        using staged_type = typename detail::ValueTraits<Sequence>::staged_type;
        using element_type = typename detail::ValueTraits<Sequence>::element_type;

        auto elements = parser.getElements();
        if (!elements) {
            return std::unexpected{elements.error()};
        }

        staged_type sequence{};
        for (auto&& element : elements.value()) {
            auto result = parse_value<element_type>(element);
            if (!result) {
                return std::unexpected{result.error()};
            }
            sequence.emplace_back(std::move(*result));
        }

        return sequence;
    }

    template<typename Map, typename Parser>
    [[nodiscard]] constexpr ParseResult<Map> parse_map(Parser& parser) {
        using staged_type = typename detail::ValueTraits<Map>::staged_type;
        using key_type = typename detail::ValueTraits<Map>::key_type;
        using mapped_type = typename detail::ValueTraits<Map>::mapped_type;

        auto members = parser.getMembers();
        if (!members) {
            return std::unexpected{members.error()};
        }

        staged_type map{};
        for (auto&& [key, value] : members.value()) {
            auto key_result = parse_value<key_type>(key);
            if (!key_result) {
                return std::unexpected{key_result.error()};
            }

            auto mapped_result = parse_value<mapped_type>(value);
            if (!mapped_result) {
                return std::unexpected{mapped_result.error()};
            }

            map.emplace_back(std::move(*key_result), std::move(*mapped_result));
        }

        return map;
    }

    template<typename T, typename Parser>
    ParseResult<T> parse_value(Parser&& parser) {
        if constexpr (IsScalar<T>) {
            return parser.template getAs<T>();
        } else if constexpr (IsSequence<T>) {
            return parse_sequence<T>(parser);
        } else if constexpr (IsMap<T>) {
            // not implemented
            // return parse_map<T>(std::forward<Parser>(parser));
        } else if constexpr (IsObject<T>) {
            // repeat
            auto object = SchemaTraits<T>::schema();
            auto result = parse_object(object, parser);
            if (!result) {
                return std::unexpected{result.error()};
            }
            return object;
        } else {
            static_assert(!sizeof(T),  "unknown Field<T>::value_type");
        }
    }

    template<typename Schema, typename Parser>
    [[nodiscard]] constexpr std::expected<decltype(SchemaTraits<Schema>::schema()), Error> parse(const std::string& contents, Parser&& parser) {
        auto schema_object = SchemaTraits<Schema>::schema();
        auto result = parse_object(schema_object, parser);
        if (!result) {
            return std::unexpected{result.error()};
        }
        return schema_object;
    }

    template<typename T, typename Schema>
    [[nodiscard]] constexpr std::expected<T, Error> parse(const std::string& contents, FileFormat format) {
        switch (format) {
            case FileFormat::YAML:
                return parse<T, Schema>(contents, YamlParser{ contents });
            default:
                STRUO_CHECK(false, "unknown file format={}", static_cast<int>(format));
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
