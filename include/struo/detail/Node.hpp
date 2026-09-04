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

    private:
        Key key_{};
        Aliases aliases_{};
        Description description_{};
    };

}
