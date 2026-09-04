#pragma once

#include "struo/definitions.hpp"
#include "struo/forward.hpp"
#include "struo/detail/Node.hpp"
#include "struo/detail/traits.hpp"

namespace struo {

    template<auto Member>
    class Field : public detail::Node<Field<Member>> {
    public:
        using base_type = detail::Node<Field<Member>>;
        using member_traits = detail::MemberTraits<decltype(Member)>;
        using value_type = typename member_traits::value_type;
        using value_traits = detail::ValueTraits<value_type>;
        using staged_type = typename value_traits::staged_type;

    public:
        template<typename... Args>
        requires OneOf<Key, Args...>
        constexpr explicit Field(Args&&... args) {
            (this->apply(std::forward<Args>(args)), ...);
        }

        [[nodiscard]] constexpr bool isStaged() const noexcept {
            return staged_value_.has_value();
        }

        [[nodiscard]] constexpr const staged_type& getStagedValue() const {
            STRUO_CHECK(isStaged());
            return staged_value_.value();
        }

        [[nodiscard]] constexpr staged_type& getStagedValue() {
            STRUO_CHECK(isStaged());
            return staged_value_.value();
        }

        constexpr void setStagedValue(staged_type value) {
            staged_value_ = std::move(value);
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
        std::optional<staged_type> staged_value_{};
        std::vector<ValueConstraint<value_type>> value_constraints_{};
        std::vector<ValueDefault<value_type>> value_defaults_{};
    };

}
