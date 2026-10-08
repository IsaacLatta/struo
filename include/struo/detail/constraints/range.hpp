#pragma once

#include <type_traits>
#include <concepts>
#include <limits>
#include <format>
#include <string>
#include <string_view>

#include "struo/Result.hpp"
#include "struo/concepts.hpp"

namespace struo::detail {

    template<auto Min, auto Max>
    requires IsValidRangeBounds<Min, Max>
    struct RangeConstraint {
        using min_type = decltype(Min);
        using max_type = decltype(Max);

        static constexpr std::string_view name() {
            return "range";
        }

        static std::string description() {
            if constexpr (HasFormatter<decltype(Min)> && HasFormatter<decltype(Max)>) {
                return std::format("between {} and {} inclusive", Min, Max);
            }
            return "inclusive range bounds";
        }

        template<typename T>
        requires HasFormatter<T> && requires(const T& value) {
            static_cast<T>(Min); static_cast<T>(Max);
            { value >= static_cast<T>(Min) && value <= static_cast<T>(Max) } -> std::convertible_to<bool>;
        }
        Result<void> operator()(const T& value) const {
            using value_type = std::remove_cvref_t<decltype(value)>;

            const auto min = static_cast<value_type>(Min);
            const auto max = static_cast<value_type>(Max);

            if(value >= min && value <= max) {
                return ok();
            }

            return err(ARGUMENT_OUT_OF_RANGE, std::format("\"{}\" constraint failed: expected between {} and {} inclusive, got {}", name(), min, max, value));
        }
    };

    template<size_t Min, size_t Max>
    requires (Min <= Max)
    struct SizeRangeConstraint {
        static constexpr std::string_view name() {
            return "size range";
        }

        static std::string description() {
            return std::format("size between {} and {} inclusive", Min, Max);
        }

        template<HasSizeApi T>
        Result<void> operator()(const T& container) const {
            const size_t size = container.size();
            if(size >= Min && size <= Max) {
                return ok();
            }
            return err(ARGUMENT_OUT_OF_RANGE, std::format("\"{}\" constraint failed: expected {}, got size {}", name(), description(), size));
        }
    };

}
