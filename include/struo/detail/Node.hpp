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
        std::optional<std::string_view> parsed_as_key_{};
        Keys aliases_{};
        Description description_{};
        Presence presence_ { OPTIONAL };
    };

}
