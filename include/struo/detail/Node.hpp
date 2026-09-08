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
        [[nodiscard]] constexpr Description getDescription() const noexcept {
            return description_;
        }

        [[nodiscard]] constexpr auto getKeys() const noexcept {
            return std::ranges::views::all(aliases_.value);
        }

    protected:
        constexpr void apply(const Description description) {
            description_ = description;
        }

        constexpr void apply(Keys aliases) {
            std::ranges::move(aliases.value, std::back_inserter(aliases_.value));
        }

    private:
        Keys aliases_{};
        Description description_{};
    };

}
