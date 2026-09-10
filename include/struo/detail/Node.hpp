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
        [[nodiscard]] constexpr std::string_view getPrimaryKey() const noexcept {
            auto keys = this->getKeys();
            return std::ranges::empty(keys) ? std::string_view{"<unnamed-field>"} : std::string_view{*std::ranges::begin(keys)};
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

    protected:
        constexpr void apply(Description description) {
            description_ = description;
        }

        constexpr void apply(Keys aliases) {
            std::ranges::move(aliases.value, std::back_inserter(aliases_.value));
        }

        constexpr void apply(Presence presence) {
            presence_ = presence;
        }

    private:
        Keys aliases_{};
        Description description_{};
        Presence presence_ { OPTIONAL };
    };

}
