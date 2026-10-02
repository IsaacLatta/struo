#pragma once

#include <algorithm>
#include <ranges>
#include <type_traits>
#include <cstdlib>
#include <iterator>

#include "struo/forward.hpp"
#include "struo/defaults.hpp"

#include "struo/detail/detail.hpp"
#include "struo/detail/asserts.hpp"
#include "struo/detail/traits.hpp"

namespace struo {

    template<auto Member>
    class Field {
    public:
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

        [[nodiscard]] constexpr std::string_view getPrimaryKey() const noexcept {
            if(parsed_as_key_) {
                return *parsed_as_key_;
            }
            return aliases_.value.empty() ? std::string_view{"<unnamed-field>"} : aliases_.value.front();
        }

        [[nodiscard]] constexpr Description getDescription() const noexcept {
            return description_;
        }

        [[nodiscard]] constexpr auto getKeys() const noexcept {
            return std::ranges::views::all(aliases_.value);
        }

        [[nodiscard]] constexpr bool is(Presence presence) const noexcept {
            return presence_ == presence;
        }

        void setPrimaryKey(std::string_view key) noexcept {
            parsed_as_key_ = key;
        }

    private:
        using DefaultResult = Result<std::optional<value_type>>;
        using DefaultFunc = std::function<DefaultResult()>;
        using ConstraintFunc = std::function<Result<void>(const value_type&)>;

    private:
        constexpr void apply(Description description) {
            description_ = description;
        }

        constexpr void apply(Keys aliases) {
            std::ranges::move(aliases.value, std::back_inserter(aliases_.value));
        }

        constexpr void apply(Presence presence) {
            presence_ = presence;
        }

        template<typename... Callables>
        constexpr void apply(Defaults<Callables...> defaults) {
            detail::apply_and_wrap_arg_func_pack<value_type, DefaultResult>(std::move(defaults), defaults_);
        }

        template<typename... Callables>
        constexpr void apply(Constraints<Callables...> constraints) {
            detail::apply_and_wrap_arg_func_pack<value_type, Result<void>>(std::move(constraints), constraints_);
        }

    private:
        Keys aliases_{};
        Description description_{};
        Presence presence_ { OPTIONAL };
        std::optional<staged_type> staged_value_{};
        std::vector<DefaultFunc> defaults_{};
        std::vector<ConstraintFunc> constraints_{};
        std::optional<std::string_view> parsed_as_key_{};
    };
}
