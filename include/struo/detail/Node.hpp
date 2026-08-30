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
        template<typename... Args>
        requires AppearsOnce<Key>
        constexpr explicit Node(Args&&... args) {
            (apply(std::forward<Args>(args)), ...);
        }

        [[nodiscard]] constexpr Key getKey() const noexcept {
            return key_;
        }

        [[nodiscard]] constexpr Description getDescription() const noexcept {
            return description_;
        }

        [[nodiscard]] constexpr std::span<const Alias> getAliases() const noexcept {
            return aliases_;
        }

        [[nodiscard]] constexpr std::span<const ObjectDefault<Derived>> getObjectDefaults() const noexcept {
            return object_defaults_;
        }

        [[nodiscard]] constexpr std::span<const ObjectConstraints<Derived>> getObjectConstraints() const noexcept {
            return object_constraints_;
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

        template<typename... Defaults>
        constexpr void apply(ObjectDefaults<Defaults...> defaults) {
            std::apply([this](auto&& callable) {
                object_defaults_.emplace_back(std::forward<decltype(callable)>(callable));
            }, std::move(defaults.values));
        }

        template<typename... Constraints>
        constexpr void apply(ObjectConstraints<Constraints...> constraints) {
            std::apply([this](auto&& callable) {
                object_constraints_.emplace_back(std::forward<decltype(callable)>(callable));
            }, std::move(constraints.values));
        }

    private:
        Key key_{};
        Aliases aliases_{};
        Description description_{};
        std::vector<ObjectDefault<Derived>> object_defaults_{};
        std::vector<ObjectConstraints<Derived>> object_constraints_{};
    };

}
