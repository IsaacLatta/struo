#pragma once

#include <expected>
#include <string>
#include <filesystem>
#include <string_view>
#include <charconv>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

#include "struo/Error.hpp"
#include "struo/Result.hpp"
#include "struo/Object.hpp"
#include "struo/Field.hpp"
#include "struo/concepts.hpp"
#include "struo/detail/context.hpp"
#include "struo/detail/traits.hpp"
#include "struo/forward.hpp"

#include "struo/parsing/YamlParser.hpp"

namespace struo::detail {

    template<typename... T>
    concept AreMutableLValueReferences = ((!std::is_const_v<std::remove_reference_t<T>> && std::is_lvalue_reference_v<T>) && ...);

    template<typename T>
    using ParseResult = Result<typename detail::ValueTraits<T>::staged_type>;

    template<typename T>
    using SchemaStage = decltype(SchemaTraits<T>::schema());

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

    template<typename T, typename StagedValue>
    constexpr Result<T> materialize_value(StagedValue&, TraversalContext&);

    template<typename UserObject, typename StagedObject>
    constexpr Result<UserObject> materialize_object(StagedObject&, TraversalContext&);

    template<typename Sequence, typename StagedValue>
    constexpr Result<Sequence> materialize_sequence(StagedValue& staged, TraversalContext& context) {
        using element_type = detail::ValueTraits<Sequence>::element_type;

        size_t index { 0u };
        Sequence sequence{};
        for(auto& element : staged) {
            auto on_exit = context.enterElement(index);

            auto result = materialize_value<element_type>(element, context);
            if(!result) {
                return result.error();
            }
            sequence.emplace_back(std::move(*result));
            ++index;
        }

        return sequence;
    }

    template<typename Map, typename StagedValue>
    constexpr Result<Map> materialize_map(StagedValue& staged, TraversalContext& context) {
        using mapped_type = detail::ValueTraits<Map>::mapped_type;
        using key_type = detail::ValueTraits<Map>::key_type;

        Map map{};
        for(auto& [staged_key, staged_mapped] : staged) {
            auto on_exit = context.enterMember(staged_key);

            auto key = materialize_value<key_type>(staged_key, context);
            if(!key) {
                return key.error();
            }

            auto mapped = materialize_value<mapped_type>(staged_mapped, context);
            if(!mapped) {
                return mapped.error();
            }

            map.emplace(std::move(*key), std::move(*mapped));
        }

        return map;
    }

    template<typename T, typename StagedValue>
    constexpr Result<T> materialize_value(StagedValue& staged, TraversalContext& context) {
        if constexpr (IsScalar<T>) {
            static_assert(std::same_as<T, StagedValue>);
            return std::move(staged);
        } else if constexpr (IsMap<T>) {
            return materialize_map<T>(staged, context);
        } else if constexpr (IsSequence<T>) {
            return materialize_sequence<T>(staged, context);
        } else if constexpr (IsObject<T>) {
            return materialize_object<T>(staged, context);
        } else {
            static_assert(!sizeof(T), "unknown type T");
        }
    }

    template<auto Member, typename UserObject>
    constexpr Result<void> materialize_field(Field<Member>& field, UserObject& object, TraversalContext& context) {
        using field_type = Field<Member>;
        using member_object_type = typename field_type::member_traits::object_type;
        using member_value_type = typename field_type::member_traits::value_type;

        static_assert(AreMutableLValueReferences<decltype(field), decltype(object)>);
        static_assert(std::same_as<member_object_type, UserObject>);

        auto on_exit = context.enterField(field.getPrimaryKey());

        std::optional<member_value_type> value{};
        if(!field.isStaged()) {
            for(const auto& default_func : field.getDefaults()) {
                auto default_value = default_func();
                if(!default_value) {
                    return append_to_err(default_value.error(), context);
                }

                if(!*default_value) {
                    continue;
                }

                value = std::move(**default_value);
                break;
            }
        } else {
            auto materialized_value = materialize_value<member_value_type>(field.getStagedValue(), context);
            if(!materialized_value) {
                return materialized_value.error();
            }
            value = std::move(*materialized_value);
        }

        if(!value) {
            if(field.is(REQUIRED)) {
                return append_to_err(
                    err(KEY_NOT_FOUND, std::format("failed to resolve field \"{}\"", field.getPrimaryKey())),
                    context);
            }
            return ok();
        }

        for(const auto& constraint : field.getConstraints()) {
            if(auto result = std::invoke(constraint, *value); !result) {
                return append_to_err(result.error(), context);
            }
        }

        object.*Member = std::move(*value);
        return ok();
    }

    template<typename UserObject, typename StagedObject>
    constexpr Result<UserObject> materialize_object(StagedObject& staged, TraversalContext& context) {
        UserObject user_object{};

        auto result = staged.forEachField([&](auto& field) -> Result<void> {
            static_assert(AreMutableLValueReferences<decltype(field)>);
            return materialize_field(field, user_object, context);
        });

        if(!result) {
            return result.error();
        }

        return user_object;
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

        auto tag_key = parser.toChild(variant_schema.getTagKey());
        if(!tag_key) {
            return append_to_err(tag_key.error(), context);
        }

        if(!*tag_key) {
            return append_to_err(err(KEY_NOT_FOUND, std::format("missing tag key \"{}\"", variant_schema.getTagKey())), context);
        }

        auto tag_key_value = (**tag_key).template getAs<std::string>();
        if(!tag_key_value) {
            return append_to_err(tag_key_value.error(), context);
        }

        auto bindings = variant_schema.getBindings();
        return std::apply([&](auto&&... binds) -> ParseResult<StdVariant> {
            std::optional<ParseResult<StdVariant>> result{};
            auto visit_binding = [&]<typename Binding>(const Binding& bind) -> bool {
                using alt_type = typename Binding::value_type;
                constexpr auto alt_index = value_traits::template index_of<alt_type>;

                if(Binding::tag != *tag_key_value) {
                    return true;
                }

                auto content_key = parser.toChild(variant_schema.getContentKey());
                if(!content_key) {
                    result = content_key.error();
                    return false;
                }

                if(!*content_key) {
                    result = err(KEY_NOT_FOUND, std::format("content key \"{}\" not found in variant", variant_schema.getContentKey()));
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

    template<typename UserObject>
    constexpr Result<UserObject> materialize(SchemaStage<UserObject>& staged) {
        TraversalContext context{};
        return materialize_object<UserObject>(staged, context);
    }
}
