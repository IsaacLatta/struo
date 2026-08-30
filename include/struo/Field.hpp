#pragma once

#include "definitions.hpp"
#include "struo/forward.hpp"
#include "struo/detail/Node.hpp"

namespace struo {

    template<typename T>
    class Field : public detail::Node<Field<T>> {
    public:
        using Base = detail::Node<Field<T>>;

    public:
        template<typename... Args>
        requires AppearsOnce<Key, Args...>
        constexpr explicit Field(Args&&... args) {
            (this->apply(std::forward<Args>(args)), ...);
        }

    private:
        using Base::apply;

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
        std::vector<ValueConstraint<T>> value_constraints_{};
        std::vector<ValueDefault<T>> value_defaults_{};
    };

}