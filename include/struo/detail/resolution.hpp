#pragma once

#include "struo/Field.hpp"
#include "struo/Result.hpp"
#include "struo/detail/context.hpp"

namespace struo::detail {

    using ResolutionContext = Context<DefinitionContext, TraversalContext>;

    template<typename T>
    Result<void> traverse_value(const T& value, ResolutionContext& context);

    template<typename Object, typename Schema>
    Result<void> resolve_object(const Object& object, const Schema& schema, ResolutionContext& context);

    // Called only for a field annotated with References<Domain>. Here `value`
    // is the identifier to look up, not an object whose children we should visit.
    // A missing optional identifier makes no reference and therefore succeeds.
    template<typename T>
    Result<void> try_resolve_reference(const T& value, const Reference& reference, const ResolutionContext& context) {
        if constexpr (IsOptional<T>) {
            return value ? try_resolve_reference(*value, reference, context) : ok();
        } else {
            // Field records the domain and the unwrapped identifier type when
            // References<Domain> is applied. Supply the address of the actual
            // final identifier to complete the lookup key. This borrows value;
            // neither the identifier nor any definition is copied or moved.
            //
            // DefinitionKey equality first checks domain and type identity,
            // then calls the stored equality function to compare the values.
            // Reusing reference.type_ also avoids generating equality functions
            // for every unannotated type encountered by the general traversal.
            const DefinitionKey key { reference.domain_id_, std::addressof(value), reference.type_ };
            if(context.getSubcontext<DefinitionContext>().contains(key)) {
                return ok();
            }

            return append_to_err(
                err(KEY_NOT_FOUND,
                    std::format("unresolved reference in domain \"{}\"", reference.domain_id_->name)),
                context.getSubcontext<TraversalContext>());
        }
    }

    // Collect definitions from one immediate member of the current object.
    // Only optional wrappers are unwrapped here. Descending into child objects
    // would incorrectly make their definitions visible to their ancestors.
    template<typename T, typename DomainRange>
    void inspect_definitions_at_level(const T& value, const DomainRange& domains, DefinitionContext& context) {
        if constexpr (IsOptional<T>) {
            if(value) {
                inspect_definitions_at_level(*value, domains, context);
            }
        } else if constexpr (IsMap<T>) {
            if constexpr (IsInstanceKey<typename T::key_type>) {
                context.addDefinitions(domains, value);
            }
        }
    }

    template<auto Member, typename Object>
    Result<void> resolve_field(const Field<Member>& field, const Object& object, ResolutionContext& context) {
        auto on_exit = context.enterField(field.getPrimaryKey());

        if(field.isReference()) {
            return try_resolve_reference(object.*Member, field.getReference(), context);
        }

        return traverse_value(object.*Member, context);
    }

    template<typename Object, typename Schema>
    Result<void> resolve_object(const Object& object, const Schema& schema, ResolutionContext& context) {
        auto& definitions = context.getSubcontext<DefinitionContext>();
        auto on_exit = context.enterObject();

        // First, publish every immediate defining map before checking any
        // references. This permits sibling references regardless of schema
        // declaration order.
        (void)schema.forEachField([&]<auto Member>(const Field<Member>& field) -> Result<void> {
            if(field.hasDefinitions()) {
                inspect_definitions_at_level(object.*Member, field.getDefinitions(), definitions);
            }
            return ok();
        });

        return schema.forEachField([&](const auto& field) -> Result<void> {
            return resolve_field(field, object, context);
        });
    }

    template<typename T>
    Result<void> traverse_value(const T& value, ResolutionContext& context) {
        if constexpr (IsOptional<T>) {
            return value ? traverse_value(*value, context) : ok();
        } else if constexpr (IsScalar<T>) {
            return ok();
        } else if constexpr (IsObject<T>) {
            return resolve_object(value, SchemaTraits<T>::schema(), context);
        } else if constexpr (IsMap<T>) {
            for(const auto& [key, mapped] : value) {
                auto on_exit = context.enterMember(key);
                if(auto result = traverse_value(key, context); !result) {
                    return result.error();
                }

                if(auto result = traverse_value(mapped, context); !result) {
                    return result.error();
                }
            }
            return ok();
        } else if constexpr (IsSequence<T>) {
            size_t index { 0 };
            for(const auto& element : value) {
                auto on_exit = context.enterElement(index);

                if(auto result = traverse_value(element, context); !result) {
                    return result.error();
                }
                index++;
            }
            return ok();
        } else if constexpr (IsVariant<T>) {
            if(value.valueless_by_exception()) {
                return append_to_err(err(INVALID_VALUE, "valueless variant"), context.getSubcontext<TraversalContext>());
            }

            const auto schema = SchemaTraits<T>::schema();
            return std::visit([&](const auto& alternative) -> Result<void> {
                using alternative_type = std::remove_cvref_t<decltype(alternative)>;

                // Visit only the active alternative. Find its tag by type,
                // because schema bindings need not follow std::variant's order.
                std::string_view tag{};
                std::apply([&](const auto&... bindings) {
                    auto try_assign_tag = [&](const auto& binding) {
                        using binding_type = std::remove_cvref_t<decltype(binding)>;

                        if constexpr (std::same_as<alternative_type, typename binding_type::value_type>) {
                            tag = binding_type::tag;
                        }
                    };

                    (try_assign_tag(bindings), ...);
                }, schema.getBindings());

                auto variant_scope = context.enterVariant(tag);
                auto content_scope = context.enterField(schema.getContentKey());
                return traverse_value(alternative, context);
            }, value);
        }
    }

    template<typename Object, typename Schema>
    Result<void> resolve(const Object& object, const Schema& schema) {
        ResolutionContext context{};
        return resolve_object(object, schema, context);
    }
}
