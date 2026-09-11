#pragma once

#include <ranges>
#include <type_traits>
#include <cstdlib>

#include "struo/Error.hpp"
#include "struo/detail/detail.hpp"
#include "struo/forward.hpp"

#include "struo/detail/StrongAlias.hpp"
#include "struo/detail/asserts.hpp"
#include "struo/detail/Node.hpp"
#include "struo/detail/traits.hpp"

namespace struo {

    template <size_t N>
    struct Str {
        char string[N];

        constexpr Str(const char (&str)[N]) {
            for (size_t i { 0 }; i < N; ++i)
                string[i] = str[i];
        }
    };

    template<auto V>
    struct ValueImpl {
        [[nodiscard]] constexpr auto operator()() const noexcept {
            return V;
        }
    };

    template<auto V>
    static inline constexpr ValueImpl<V> Value{};

    template<Str Key>
    struct FromEnvDefault {
        template<typename T>
        constexpr Result<std::optional<T>> operator()() const noexcept {
            const char* value_raw = std::getenv(Key.string);
            if(!value_raw) {
                return err(KEY_NOT_FOUND, std::format("env variable \"{}\" not set", Key.string));
            }
            std::string value_as_str { value_raw };

            auto value = detail::from_string<T>(value_as_str);
            if(!value) {
                return err(WRONG_TYPE, std::format("fail to convert env variable \"{}\" to type T", Key.string));
            }
            return std::optional<T>{*value};
        }
    };

    template<Str Key>
    static inline constexpr auto FromEnv { FromEnvDefault<Key>{} };

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
        requires IsOneOf<Keys, Args...>
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
        using ConstraintFunc = std::function<Result<void>(const value_type&)>;

    private:
        template<typename... Callables>
        constexpr void apply(Defaults<Callables...> defaults) {
            detail::apply_and_wrap_arg_func_pack<value_type, DefaultResult>(std::move(defaults), defaults_);
        }

        template<typename... Callables>
        constexpr void apply(Constraints<Callables...> constraints) {
            detail::apply_and_wrap_arg_func_pack<value_type, Result<void>>(std::move(constraints), constraints_);
        }

    private:
        std::optional<staged_type> staged_value_{};
        std::vector<DefaultFunc> defaults_{};
        std::vector<ConstraintFunc> constraints_{};
    };

}
