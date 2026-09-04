#pragma once

#include <expected>
#include <string>
#include <filesystem>
#include <string_view>

#include <magic_enum/magic_enum.hpp>

#include "struo/Result.hpp"
#include "struo/Object.hpp"
#include "struo/Field.hpp"
#include "struo/concepts.hpp"
#include "struo/definitions.hpp"

namespace struo::detail {

    template<typename T>
    requires std::is_enum_v<T>
    [[nodiscard]] constexpr std::string_view enum_name(T t) {
        return magic_enum::enum_name(t);
    }

    template<typename... T>
    concept AreMutableLValueReferences = ((!std::is_const_v<std::remove_reference_t<T>> && std::is_lvalue_reference_v<T>) && ...);

    template<typename T>
    using ParseResult = Result<typename detail::ValueTraits<T>::staged_type>;

    template<typename T>
    using SchemaStage = decltype(SchemaTraits<T>::schema());

    template<typename T, typename Parser>
    ParseResult<T> parse_value(Parser&&);

    template<typename Field, typename Parser>
    Result<void> parse_field(Field& field, Parser& parser) {
        static_assert(AreMutableLValueReferences<decltype(field), decltype(parser)>);

        using field_type = std::remove_cvref_t<decltype(field)>;
        using value_type = typename field_type::value_type;

        auto child = parser.toChild(field.getKey().value);
        if (!child) {
            return child.error();
        }

        if (!*child) {
            return ok();
        }

        auto value = parse_value<value_type>(**child);
        if (!value) {
            return value.error();
        }
        field.setStagedValue(std::move(*value));

        return ok();
    }

    template<typename Object, typename Parser>
    constexpr Result<void> parse_object(Object& object, Parser& parser) {
        static_assert(AreMutableLValueReferences<decltype(object), decltype(parser)>);

        return object.forEachField([&](auto& field) -> Result<void> {
            static_assert(AreMutableLValueReferences<decltype(field)>);

            return parse_field(field, parser);
        });
    }

    template<typename T, typename StagedValue>
    constexpr Result<T> materialize_value(StagedValue&);

    template<typename UserObject, typename StagedObject>
    constexpr Result<UserObject> materialize_object(StagedObject&);

    template<typename Sequence, typename StagedValue>
    constexpr Result<Sequence> materialize_sequence(StagedValue& staged) {
        using element_type = detail::ValueTraits<Sequence>::element_type;

        Sequence sequence{};
        for(auto& element : staged) {
            auto result = materialize_value<element_type>(element);
            if(!result) {
                return result.error();
            }
            sequence.emplace_back(std::move(*result));
        }

        return sequence;
    }

    template<typename Map, typename StagedValue>
    constexpr Result<Map> materialize_map(StagedValue& staged) {
        using mapped_type = detail::ValueTraits<Map>::mapped_type;
        using key_type = detail::ValueTraits<Map>::key_type;

        Map map{};
        for(auto& [staged_key, staged_mapped] : staged) {
            auto key = materialize_value<key_type>(staged_key);
            if(!key) {
                return key.error();
            }

            auto mapped = materialize_value<mapped_type>(staged_mapped);
            if(!mapped) {
                return mapped.error();
            }

            map.emplace(std::move(*key), std::move(*mapped));
        }

        return map;
    }

    template<typename T, typename StagedValue>
    constexpr Result<T> materialize_value(StagedValue& staged) {
        if constexpr (IsScalar<T>) {
            static_assert(std::same_as<T, StagedValue>);
            return std::move(staged);
        } else if constexpr (IsMap<T>) {
            return materialize_map<T>(staged);
        } else if constexpr (IsSequence<T>) {
            return materialize_sequence<T>(staged);
        } else if constexpr (IsObject<T>) {
            return materialize_object<T>(staged);
        } else {
            static_assert(!sizeof(T), "unknown type T");
        }
    }

    template<auto Member, typename UserObject>
    constexpr Result<void> materialize_field(Field<Member>& field, UserObject& object) {
        using field_type = Field<Member>;
        using member_object_type = typename field_type::member_traits::object_type;
        using member_value_type = typename field_type::member_traits::value_type;

        static_assert(AreMutableLValueReferences<decltype(field), decltype(object)>);
        static_assert(std::same_as<member_object_type, UserObject>);

        if(!field.isStaged()) {
            return ok();
        }

        auto value = materialize_value<member_value_type>(field.getStagedValue());
        if(!value) {
            return value.error();
        }

        object.*Member = std::move(*value);

        return ok();
    }

    template<typename UserObject, typename StagedObject>
    constexpr Result<UserObject> materialize_object(StagedObject& staged) {
        UserObject user_object{};

        auto result = staged.forEachField([&](auto& field) -> Result<void> {
            static_assert(AreMutableLValueReferences<decltype(field)>);
            return materialize_field(field, user_object);
        });

        if(!result) {
            return result.error();
        }

        return user_object;
    }

    template<typename Sequence, typename Parser>
    constexpr ParseResult<Sequence> parse_sequence(Parser& parser) {
        static_assert(AreMutableLValueReferences<decltype(parser)>);

        using staged_type = typename detail::ValueTraits<Sequence>::staged_type;
        using element_type = typename detail::ValueTraits<Sequence>::element_type;

        auto elements = parser.getElements();
        if (!elements) {
            return elements.error();
        }

        staged_type sequence{};
        for (auto& element : elements.value()) {
            static_assert(AreMutableLValueReferences<decltype(element)>);

            auto result = parse_value<element_type>(element);
            if (!result) {
                return result.error();
            }
            sequence.emplace_back(std::move(*result));
        }

        return sequence;
    }

    template<typename Map, typename Parser>
    constexpr ParseResult<Map> parse_map(Parser& parser) {
        static_assert(AreMutableLValueReferences<decltype(parser)>);

        using staged_type = typename detail::ValueTraits<Map>::staged_type;
        using key_type = typename detail::ValueTraits<Map>::key_type;
        using mapped_type = typename detail::ValueTraits<Map>::mapped_type;

        auto members = parser.getMembers();
        if (!members) {
            return members.error();
        }

        staged_type map{};
        for (auto& [key, value] : members.value()) {
            static_assert(AreMutableLValueReferences<decltype((key)), decltype((value))>);

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
            return parse_map<T>(parser);
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
    constexpr Result<SchemaStage<Schema>> parse(Parser&& parser) {
        auto schema_object = SchemaTraits<Schema>::schema();
        auto result = parse_object(schema_object, parser);
        if (!result) {
            return result.error();
        }
        return schema_object;
    }

    template<typename UserObject>
    constexpr Result<UserObject> materialize(SchemaStage<UserObject>& staged) {
        return materialize_object<UserObject>(staged);
    }
}
