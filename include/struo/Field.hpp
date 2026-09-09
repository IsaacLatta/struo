#pragma once

#include "struo/definitions.hpp"
#include "struo/detail/StrongAlias.hpp"
#include "struo/forward.hpp"
#include "struo/detail/Node.hpp"
#include "struo/detail/traits.hpp"
#include <type_traits>

namespace struo {

    template<auto V>
    struct ValueImpl {
        [[nodiscard]] constexpr auto operator()() const noexcept {
            return V;
        }
    };

    template<auto V>
    static inline constexpr ValueImpl<V> Value{};

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
        requires OneOf<Keys, Args...>
        constexpr explicit Field(Args&&... args) {
            (this->apply(std::forward<Args>(args)), ...);
        }

        [[nodiscard]] constexpr bool isStaged() const noexcept {
            return staged_value_.has_value();
        }

        [[nodiscard]] constexpr const staged_type& getStagedValue() const noexcept {
            STRUO_ASSERT(isStaged());
            return staged_value_.value();
        }

        [[nodiscard]] constexpr staged_type& getStagedValue() noexcept {
            STRUO_ASSERT(isStaged());
            return staged_value_.value();
        }

        [[nodiscard]] constexpr auto getDefaults() const noexcept {
            return std::views::all(defaults_);
        }

        [[nodiscard]] constexpr auto getConstraints() const noexcept {
            return std::views::all(constraints_);
        }

        constexpr void setStagedValue(staged_type value) {
            staged_value_ = std::move(value);
        }

    private:
        using base_type::apply;

        using DefaultResult = Result<std::optional<value_type>>;
        using DefaultFunc = std::function<DefaultResult()>;
        using ConstraintFunc = std::function<Result<void>()>;

    private:
        template<typename... Callables>
        constexpr void apply(Defaults<Callables...> defaults) {
            detail::apply_and_wrap_arg_func_pack<DefaultResult>(std::move(defaults), defaults_);
        }

        template<typename... Callables>
        constexpr void apply(Constraints<Callables...> constraints) {
            detail::apply_and_wrap_arg_func_pack<ConstraintFunc>(std::move(constraints), constraints_);
        }

    private:
        std::optional<staged_type> staged_value_{};
        std::vector<DefaultFunc> defaults_{};
        std::vector<ConstraintFunc> constraints_{};
    };

}
