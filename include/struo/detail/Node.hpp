#pragma once

#include <algorithm>
#include <ranges>
#include <span>

#include "struo/forward.hpp"
#include "struo/concepts.hpp"

namespace struo::detail {

    template<typename Derived>
    class Node {
    public:
        [[nodiscard]] constexpr Key getKey() const noexcept {
            return key_;
        }

        [[nodiscard]] constexpr Description getDescription() const noexcept {
            return description_;
        }

        [[nodiscard]] constexpr auto getAliases() const noexcept {
            return std::ranges::views::all(aliases_);
        }

        // [[nodiscard]] constexpr auto getObjectDefaults() const noexcept {
        //     return std::views::all(object_defaults_);
        // }

        // [[nodiscard]] constexpr auto getObjectConstraints() const noexcept {
        //     return std::views::all(object_constraints_);
        // }

    protected:
        constexpr void apply(const Key key) {
            key_ = key;
        }

        constexpr void apply(const Description description) {
            description_ = description;
        }

        constexpr void apply(Aliases aliases) {
            std::ranges::move(aliases, std::back_inserter(aliases_));
        }

        // template<typename... Defaults>
        // constexpr void apply(ObjectDefaults<Defaults...> defaults) {
        //     apply_arg_pack(std::move(defaults), object_defaults_);
        // }

        // template<typename... Constraints>
        // constexpr void apply(ObjectConstraints<Constraints...> constraints) {
        //     apply_arg_pack(std::move(constraints), object_constraints_);
        // }

    private:
        Key key_{};
        Aliases aliases_{};
        Description description_{};
        // std::vector<ObjectDefault<Derived>> object_defaults_{};
        // std::vector<ObjectConstraints<Derived>> object_constraints_{};
    };

}
