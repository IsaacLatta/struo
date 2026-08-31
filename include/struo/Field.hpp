#pragma once

#include "struo/definitions.hpp"
#include "struo/forward.hpp"
#include "struo/detail/Node.hpp"
#include "struo/detail/traits.hpp"

namespace struo {

    template<auto Member>
    requires IsSupportedField<decltype(Member)>
    class Field : public detail::Node<Field<Member>> {
    public:
        using base_type = detail::Node<Field<Member>>;
        using member_type = decltype(Member);
        using member_traits = typename detail::MemberTraits<member_type>;
        using value_type = typename member_traits::value_type;
        using value_traits = detail::ValueTraits<value_type>;

    public:
        template<typename... Args>
        requires OneOf<Key, Args...>
        constexpr explicit Field(Args&&... args) {
            (this->apply(std::forward<Args>(args)), ...);
        }

    private:
        using base_type::apply;

    private:
        template<typename... Constraints>
        constexpr void apply(ValueConstraints<Constraints...> constraints) {
            detail::apply_arg_pack(std::move(constraints), value_constraints_);
        }

        template<typename... Defaults>
        constexpr void apply(ValueDefaults<Defaults...> defaults) {
            detail::apply_arg_pack(std::move(defaults), value_defaults_);
        }

    private:
        std::vector<ValueConstraint<value_type>> value_constraints_{};
        std::vector<ValueDefault<value_type>> value_defaults_{};
    };

}