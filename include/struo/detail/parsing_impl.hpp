#pragma once

#include <expected>
#include <string>
#include <filesystem>
#include <string_view>
#include <charconv>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>
#include <variant>

#include "struo/Error.hpp"
#include "struo/Result.hpp"
#include "struo/Object.hpp"
#include "struo/Field.hpp"
#include "struo/concepts.hpp"
#include "struo/detail/context.hpp"
#include "struo/detail/traits.hpp"
#include "struo/forward.hpp"

#include "struo/parsing/YamlParser.hpp"

#include "struo/detail/detail.hpp"

namespace struo::detail {

    template<typename T>
    using ParseResult = Result<typename ValueTraits<T>::staged_type>;

    template<typename T, typename Parser>
    ParseResult<T> parse_value(Parser&&, TraversalContext&);

    template<typename StdVariant, typename Parser>
    ParseResult<StdVariant> parse_variant(Parser&, TraversalContext&);

    template<typename Field, typename Parser>
    Result<void> parse_field(Field& field, Parser& parser, TraversalContext& context) {
        static_assert(AreMutableLValueReferences<decltype(field), decltype(parser)>);

        using field_type = std::remove_cvref_t<decltype(field)>;
        using value_type = typename field_type::value_type;

        for(const auto key : field.getKeys()) {
            auto on_exit = context.enterField(key);
            auto child = parser.toChild(key);
            if (!child) {
                return append_to_err(child.error(), context);
            }

            if (!*child) {
                continue;
            }

            field.setPrimaryKey(key);

            auto value = parse_value<value_type>(**child, context);
            if (!value) {
                return value.error();
            }

            field.setStagedValue(std::move(*value));
            return ok();
        }

        return ok();
    }

    template<typename Object, typename Parser>
    constexpr Result<void> parse_object(Object& object, Parser& parser, TraversalContext& context) {
        static_assert(AreMutableLValueReferences<decltype(object), decltype(parser)>);

        return object.forEachField([&](auto& field) -> Result<void> {
            static_assert(AreMutableLValueReferences<decltype(field)>);

            return parse_field(field, parser, context);
        });
    }

    template<typename Sequence, typename Parser>
    constexpr ParseResult<Sequence> parse_sequence(Parser& parser, TraversalContext& context) {
        static_assert(AreMutableLValueReferences<decltype(parser)>);

        using staged_type = typename detail::ValueTraits<Sequence>::staged_type;
        using element_type = typename detail::ValueTraits<Sequence>::element_type;

        auto elements = parser.getElements();
        if (!elements) {
            return append_to_err(elements.error(), context);
        }

        size_t index { 0u };
        staged_type sequence{};
        for (auto& element : elements.value()) {
            static_assert(AreMutableLValueReferences<decltype(element)>);

            auto on_exit = context.enterElement(index);

            auto result = parse_value<element_type>(element, context);
            if (!result) {
                return result.error();
            }
            sequence.emplace_back(std::move(*result));

            ++index;
        }

        return sequence;
    }

    template<typename Map, typename Parser>
    constexpr ParseResult<Map> parse_map(Parser& parser, TraversalContext& context) {
        static_assert(AreMutableLValueReferences<decltype(parser)>);

        using staged_type = typename detail::ValueTraits<Map>::staged_type;
        using key_type = typename detail::ValueTraits<Map>::key_type;
        using mapped_type = typename detail::ValueTraits<Map>::mapped_type;

        auto members = parser.getMembers();
        if (!members) {
            return append_to_err(members.error(), context);
        }

        staged_type map{};
        for (auto& [key, value] : members.value()) {
            static_assert(AreMutableLValueReferences<decltype((key)), decltype((value))>);

            auto key_result = parse_value<key_type>(key, context);
            if (!key_result) {
                return key_result.error();
            }

            auto on_exit = context.enterMember(*key_result);

            auto mapped_result = parse_value<mapped_type>(value, context);
            if (!mapped_result) {
                return mapped_result.error();
            }

            map.emplace_back(std::move(*key_result), std::move(*mapped_result));
        }

        return map;
    }

    template<typename StdVariant, typename Parser>
    ParseResult<StdVariant> parse_variant(Parser& parser, TraversalContext& context) {
        using value_traits = detail::ValueTraits<StdVariant>;
        using staged_type = typename value_traits::staged_type;

        auto variant_schema = SchemaTraits<StdVariant>::schema();

        auto tag_key_value = [&]() -> Result<std::string> {
            auto tag_scope = context.enterField(variant_schema.getTagKey());
            auto tag_key = parser.toChild(variant_schema.getTagKey());
            if(!tag_key) {
                return append_to_err(tag_key.error(), context);
            }

            if(!*tag_key) {
                return append_to_err(err(KEY_NOT_FOUND, std::format("missing tag key \"{}\"", variant_schema.getTagKey())), context);
            }

            auto value = (**tag_key).template getAs<std::string>();
            if(!value) {
                return append_to_err(value.error(), context);
            }
            return value;
        }();
        if(!tag_key_value) {
            return tag_key_value.error();
        }

        const auto& bindings = variant_schema.getBindings();
        return std::apply([&](auto&&... binds) -> ParseResult<StdVariant> {
            std::optional<ParseResult<StdVariant>> result{};
            auto visit_binding = [&]<typename Binding>(const Binding& bind) -> bool {
                using alt_type = typename Binding::value_type;
                constexpr auto alt_index = value_traits::template index_of<alt_type>;

                if(Binding::tag != *tag_key_value) {
                    return true;
                }

                auto variant_scope = context.enterVariant(Binding::tag);
                auto content_scope = context.enterField(variant_schema.getContentKey());
                auto content_key = parser.toChild(variant_schema.getContentKey());
                if(!content_key) {
                    result = append_to_err(content_key.error(), context);
                    return false;
                }

                if(!*content_key) {
                    result = append_to_err(err(KEY_NOT_FOUND, std::format("content key \"{}\" not found in variant", variant_schema.getContentKey())), context);
                    return false;
                }

                auto value_result = parse_value<alt_type>(**content_key, context);
                if(!value_result) {
                    result.emplace(std::move(value_result.error()));
                } else {
                    result.emplace(staged_type{ std::in_place_index<alt_index>, std::move(*value_result)});
                }

                return false;
            };

            (visit_binding(binds) && ...);

            if(!result.has_value()) {
                auto tag_scope = context.enterField(variant_schema.getTagKey());
                return append_to_err(
                    err(INVALID_ARGUMENT, std::format("value \"{}\" did not match any bindings", *tag_key_value)),
                    context);
            }
            return std::move(*result);
        }, bindings);
    }

    template<typename T, typename Parser>
    ParseResult<T> parse_value(Parser&& parser, TraversalContext& context) {
        if constexpr (IsScalar<T>) {
            auto result = parser.template getAs<T>();
            if(!result) {
                return append_to_err(result.error(), context);
            }
            return result;
        } else if constexpr (IsOptional<T>) {
            using inner_type = typename ValueTraits<T>::value_type;
            return parse_value<inner_type>(std::forward<Parser>(parser), context);
        } else if constexpr (IsSequence<T>) {
            return parse_sequence<T>(parser, context);
        } else if constexpr (IsMap<T>) {
            return parse_map<T>(parser, context);
        } else if constexpr (IsVariant<T>) {
            return parse_variant<T>(parser, context);
        } else if constexpr (IsObject<T>) {
            auto object = SchemaTraits<T>::schema();
            auto result = parse_object(object, parser, context);
            if (!result) {
                return result.error();
            }
            return object;
        } else {
            static_assert(!sizeof(T),  "unknown Field<T>::value_type");
        }
    }

    template<typename Schema, typename Parser>
    constexpr Result<SchemaStage<Schema>> parse(Parser&& parser) {
        TraversalContext context{};
        auto schema_object = SchemaTraits<Schema>::schema();
        auto result = parse_object(schema_object, parser, context);
        if (!result) {
            return result.error();
        }
        return schema_object;
    }

}
