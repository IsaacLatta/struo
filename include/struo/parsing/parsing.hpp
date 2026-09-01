#pragma once

#include <expected>
#include <string>
#include <filesystem>
#include <magic_enum/magic_enum.hpp>

#include <yaml-cpp/yaml.h>

#include "struo/Error.hpp"
#include "struo/Object.hpp"
#include "struo/concepts.hpp"
#include "struo/definitions.hpp"

#include "struo/detail/detail.hpp"
#include "struo/detail/traits.hpp"

namespace struo {

    class YamlParser {
    public:
        explicit constexpr YamlParser(YAML::Node node)
            : node_(std::move(node))  {}

        template<typename T>
        [[nodiscard]] std::expected<T, Error> getAs() const {
            return tryParse<T>([this]() -> T {
                return node_.as<T>();
            }, YAML::NodeType::Scalar);
        }

        [[nodiscard]] std::expected<std::optional<YamlParser>, Error> toChild(std::string_view name) const {
            return tryParse<std::optional<YamlParser>>([&]() -> std::optional<YamlParser> {
                const auto child = node_[name];
                if (!child) {
                    return std::nullopt;
                }
                return YamlParser { child };
            });
        }

        [[nodiscard]] std::expected<std::vector<std::pair<YamlParser, YamlParser>>, Error> getMembers() const {
            return tryParse<std::vector<std::pair<YamlParser, YamlParser>>>([this] {
                std::vector<std::pair<YamlParser, YamlParser>> parsers{};
                for (auto it = node_.begin(); it != node_.end(); ++it) {
                    parsers.emplace_back(it->first, it->second);
                }
                return parsers;
            }, YAML::NodeType::Map);
        }

        [[nodiscard]] std::expected<std::vector<YamlParser>, Error> getElements() const {
            return tryParse<std::vector<YamlParser>>([this]() -> std::vector<YamlParser> {
                std::vector<YamlParser> parsers{};
                for (size_t i { 0 }; i < node_.size(); ++i) {
                    parsers.emplace_back(node_[i]);
                }
                return parsers;
            }, YAML::NodeType::Sequence);
        }

    private:
        template<typename T, typename Callable>
        requires HasFunctionSignature<Callable, T()>
        [[nodiscard]] std::expected<T, Error> tryParse(Callable&& callable, std::optional<YAML::NodeType::value> expected_type = std::nullopt) const {
            try {
                if (expected_type) {
                    if (node_.Type() != *expected_type) {
                        return err(WRONG_TYPE, std::format("expected {}, got {}", detail::enum_name(*expected_type), detail::enum_name(node_.Type())));
                    }
                }

                return std::invoke(std::forward<Callable>(callable));
            } catch (const YAML::ParserException& e) {
                return err(SYNTAX_ERROR, e.what());
            } catch (const YAML::BadConversion& e) {
                return err(INVALID_VALUE, e.what());
            } catch (const YAML::BadSubscript& e) {
                return err(WRONG_TYPE, e.what());
            } catch (const YAML::Exception& e) {
                return err(PARSE_ERROR, e.what());
            }
        }

    private:
        YAML::Node node_{};
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
