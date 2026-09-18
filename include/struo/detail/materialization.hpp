#pragma once

#include <expected>
#include <string>
#include <filesystem>
#include <string_view>
#include <charconv>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>
#include <variant>

#include "struo/forward.hpp"
#include "struo/Error.hpp"
#include "struo/Result.hpp"
#include "struo/Field.hpp"
#include "struo/concepts.hpp"
#include "struo/detail/context.hpp"
#include "struo/detail/traits.hpp"

#include "struo/detail/detail.hpp"

namespace struo::detail {

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

    template<typename Variant, size_t I = 0, typename StagedValue>
    constexpr Result<Variant> materialize_variant(StagedValue& staged, TraversalContext& context) {
        if constexpr (I == std::variant_size_v<Variant>) {
            return append_to_err(err(INVALID_VALUE, "invalid staged variant index"), context);
        } else {
            if (staged.index() != I) {
                return materialize_variant<Variant, I + 1>(staged, context);
            }

            using alt_type = std::variant_alternative_t<I, Variant>;

            auto variant_schema = SchemaTraits<Variant>::schema();
            std::string_view binding_tag;
            std::apply([&](const auto&... bindings) {
                auto find_binding = [&]<typename Binding>(const Binding&) {
                    if constexpr (std::same_as<alt_type, typename Binding::value_type>) {
                        binding_tag = Binding::tag;
                        return true;
                    }
                    return false;
                };
                (find_binding(bindings) || ...);
            }, variant_schema.getBindings());

            auto variant_scope = context.enterVariant(binding_tag);
            auto content_scope = context.enterField(variant_schema.getContentKey());

            auto value = materialize_value<alt_type>(std::get<I>(staged), context);

            if(!value) {
                return err(value);
            }

            return Variant { std::in_place_index<I>, std::move(*value) };
        }
    }

    template<typename T, typename StagedValue>
    constexpr Result<T> materialize_value(StagedValue& staged, TraversalContext& context) {
        if constexpr (IsScalar<T>) {
             static_assert(std::same_as<T, StagedValue>);
            return std::move(staged);
        } else if constexpr (IsOptional<T>) {
            using inner_type = typename ValueTraits<T>::value_type;

            auto value = materialize_value<inner_type>(staged, context);
            if(!value) {
                return err(value);
            }

            return T { std::in_place, std::move(*value) };
        } else if constexpr (IsMap<T>) {
            return materialize_map<T>(staged, context);
        } else if constexpr (IsSequence<T>) {
            return materialize_sequence<T>(staged, context);
        } else if constexpr (IsObject<T>) {
            return materialize_object<T>(staged, context);
        } else if constexpr (IsVariant<T>) {
            return materialize_variant<T>(staged, context);
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

    template<typename UserObject>
    constexpr Result<UserObject> materialize(SchemaStage<UserObject>& staged) {
        TraversalContext context{};
        return materialize_object<UserObject>(staged, context);
    }
}
