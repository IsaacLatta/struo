#pragma once

#include <string>
#include <filesystem>

#include <magic_enum/magic_enum.hpp>
#include <yaml-cpp/yaml.h>

#include "struo/Result.hpp"
#include "struo/Object.hpp"
#include "struo/concepts.hpp"
#include "struo/definitions.hpp"

#include "struo/parsing/YamlParser.hpp"

#include "struo/detail/detail.hpp"
#include "struo/detail/traits.hpp"

namespace struo {

    template<typename T>
    using ParseResult = Result<typename detail::ValueTraits<T>::staged_type>;

    template<typename T>
    using SchemaStage = decltype(SchemaTraits<T>::schema());

    template<typename T, typename Parser>
    ParseResult<T> parse_value(Parser&&);

    template<typename Field, typename Parser>
    Result<void> parse_field(Field& field, Parser& parser) {
        using field_type = std::remove_cvref_t<decltype(field)>;
        using value_type = typename field_type::value_type;

        auto child = parser.toChild(field.getKey().value);
        if (!child) {
            return child.error();
        }

        if (!*child) {
            return {};
        }

        auto value = parse_value<value_type>(**child);
        if (!value) {
            return value.error();
        }
        field.setStagedValue(std::move(*value));

        return {};
    }

    template<typename Object, typename Parser>
    [[nodiscard]] constexpr Result<void> parse_object(Object& object, Parser& parser) {
        return object.forEachField([&](auto& field) {
            return parse_field(field, parser);
        });
    }

    template<typename Sequence, typename Parser>
    [[nodiscard]] constexpr ParseResult<Sequence> parse_sequence(Parser& parser) {
        using staged_type = typename detail::ValueTraits<Sequence>::staged_type;
        using element_type = typename detail::ValueTraits<Sequence>::element_type;

        auto elements = parser.getElements();
        if (!elements) {
            return elements.error();
        }

        staged_type sequence{};
        for (auto&& element : elements.value()) {
            auto result = parse_value<element_type>(element);
            if (!result) {
                return result.error();
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
            return members.error();
        }

        staged_type map{};
        for (auto&& [key, value] : members.value()) {
            auto key_result = parse_value<key_type>(key);
            if (!key_result) {
                return key_result.error();
            }

            auto mapped_result = parse_value<mapped_type>(value);
            if (!mapped_result) {
                return mapped_result.error();
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
            return parse_map<T>(std::forward<Parser>(parser));
        } else if constexpr (IsObject<T>) {
            auto object = SchemaTraits<T>::schema();
            auto result = parse_object(object, parser);
            if (!result) {
                return result.error();
            }
            return object;
        } else {
            static_assert(!sizeof(T),  "unknown Field<T>::value_type");
        }
    }

    template<typename Schema, typename Parser>
    [[nodiscard]] constexpr Result<SchemaStage<Schema>> parse(Parser&& parser) {
        auto schema_object = SchemaTraits<Schema>::schema();
        auto result = parse_object(schema_object, parser);
        if (!result) {
            return result.error();
        }
        return schema_object;
    }

    template<typename T, typename Schema>
    [[nodiscard]] constexpr Result<SchemaStage<Schema>> parse_yaml(const std::filesystem::path& path) {
        try {
            const auto node = YAML::LoadFile(path.string());
            return parse<T, Schema>(YamlParser { node });
        } catch (const YAML::Exception& e) {
            return err(PARSE_ERROR, std::format("malformed yaml file: {}", e.what()));
        }
    }

    template<typename T, typename Schema>
    [[nodiscard]] constexpr Result<T> load(const std::filesystem::path& path) {
        const auto file_format = detail::file_format_from_path(path);
        if (!file_format) {
            return file_format.error();
        }

        switch (*file_format) {
            case FileFormat::YAML:
                return parse_yaml<T, Schema>(path);
            default:
                STRUO_CHECK(false, "unknown file format={}", static_cast<int>(*file_format));
        }
    }

}
