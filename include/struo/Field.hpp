#pragma once

#include <algorithm>
#include <ranges>
#include <type_traits>
#include <cstdlib>
#include <iterator>

#include "struo/concepts.hpp"
#include "struo/forward.hpp"
#include "struo/defaults.hpp"

#include "struo/detail/detail.hpp"
#include "struo/detail/asserts.hpp"
#include "struo/detail/traits.hpp"

namespace struo {

    template<auto Member>
    requires HasMemberTraits<decltype(Member)> && std::is_member_object_pointer_v<decltype(Member)> && (Member != nullptr)
    class Field {
    public:
        using member_traits = detail::MemberTraits<decltype(Member)>;
        using value_type = typename member_traits::value_type;
        using value_traits = detail::ValueTraits<value_type>;
        using staged_type = typename value_traits::staged_type;

    public:
        template<typename... Args>
        requires IsOneOf<Keys, Args...> && AllAppearAtMostOnce<Args...>
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

        [[nodiscard]] constexpr auto getTransforms() const noexcept {
            return std::views::all(transforms_);
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
        using TransformFunc = std::function<Result<value_type>(const value_type&)>;

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
        requires (CanProduceResult<Callables, value_type, DefaultResult> && ...)
        constexpr void apply(Defaults<Callables...> defaults) {
            detail::apply_and_wrap_arg_func_pack<value_type, DefaultResult>(std::move(defaults), defaults_);
        }

        template<typename... Callables>
        requires (CanProduceResult<typename detail::KeywordTraits<detail::ConstraintOperation, Callables>::callable_type,
            value_type, Result<void>, const value_type&> && ...)
        constexpr void apply(Constraints<Callables...> constraints) {
            detail::apply_and_wrap_arg_func_pack<value_type, Result<void>, detail::ConstraintOperation>(std::move(constraints), constraints_);
        }

        template<typename... Callables>
        requires (CanProduceResult<typename detail::KeywordTraits<detail::TransformOperation, Callables>::callable_type,
            value_type, Result<value_type>, const value_type&> && ...)
        constexpr void apply(Transforms<Callables...> transforms) {
            detail::apply_and_wrap_arg_func_pack<value_type, Result<value_type>, detail::TransformOperation>(std::move(transforms), transforms_);
        }

    private:
        Keys aliases_{};
        Description description_{};
        Presence presence_ { OPTIONAL };
        std::optional<std::string_view> parsed_as_key_{};
        std::optional<staged_type> staged_value_{};
        std::vector<DefaultFunc> defaults_{};
        std::vector<TransformFunc> transforms_{};
        std::vector<ConstraintFunc> constraints_{};
    };
}

namespace struo::detail {

template<auto Member>
struct IsFieldImpl<Field<Member>> : std::true_type {};

} // namespace struo::detail
