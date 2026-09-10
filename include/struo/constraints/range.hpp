#pragma once

#include <type_traits>
#include <concepts>
#include <limits>
#include <format>
#include <string>
#include <string_view>

#include "struo/Result.hpp"

namespace struo {

    template<auto Min, auto Max>
    requires (std::same_as<decltype(Min), decltype(Max)> && (Min <= Max))
    struct RangeConstraint {
        static constexpr std::string_view name() {
            return "range";
        }

        static std::string description() {
            return std::format("between {} and {} inclusive", Min, Max);
        }

        using min_type = decltype(Min);
        using max_type = decltype(Max);

        Result<void> operator()(const auto& value) const {
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
    struct SizeRangeConstraint {
        static constexpr std::string_view name() {
            return "size range";
        }

        static std::string description() {
            return std::format("size between {} and {} inclusive", Min, Max);
        }

        Result<void> operator()(const auto& container) const {
            const size_t size = container.size();
            if(size >= Min && size <= Max) {
                return ok();
            }
            return err(ARGUMENT_OUT_OF_RANGE, std::format("\"{}\" constraint failed: expected {}, got size {}", name(), description(), size));
        }
    };

    template<auto Min, auto Max>
    inline constexpr auto Range { RangeConstraint<Min, Max>{} };

    template<auto Min>
    inline constexpr auto AtLeast { RangeConstraint<Min, std::numeric_limits<decltype(Min)>::max()>{} };

    template<auto Max>
    inline constexpr auto AtMost { RangeConstraint<std::numeric_limits<decltype(Max)>::lowest(), Max>{} };

    template<size_t Min, size_t Max>
    inline constexpr auto SizeRange { SizeRangeConstraint<Min, Max>{} };

    template<size_t Min>
    inline constexpr auto SizeAtLeast { SizeRangeConstraint<Min, std::numeric_limits<size_t>::max()>{} };

    template<size_t Max>
    inline constexpr auto SizeAtMost { SizeRangeConstraint<std::numeric_limits<size_t>::min(), Max>{} };

    template<size_t N>
    inline constexpr auto SizeExactly { SizeRangeConstraint<N, N>{} };

    inline constexpr auto NotEmpty { SizeAtLeast<1> };
}
