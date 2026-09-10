#pragma once

#include <type_traits>
#include <concepts>
#include <limits.h>

#include "struo/Result.hpp"

namespace struo {

    template<auto Min, auto Max>
    requires (std::same_as<decltype(Min), decltype(Max)> && (Min <= Max))
    struct RangeConstraint {
        using min_type = decltype(Min);
        using max_type = decltype(Max);

        Result<void> operator()(const auto& value) const {
            using value_type = std::remove_cvref_t<decltype(value)>;

            const auto min = static_cast<value_type>(Min);
            const auto max = static_cast<value_type>(Max);

            if(value >= min && value <= max) {
                return ok();
            }

            return err(ARGUMENT_OUT_OF_RANGE, std::format("range constraint failed: !(min={} <= value={} <= max={})", min, value, max));
        }
    };

    template<size_t Min, size_t Max>
    struct SizeRangeConstraint {
        Result<void> operator()(const auto& container) const {
            const size_t size = container.size();
            if(size >= Min && size <= Max) {
                return ok();
            }
            return err(ARGUMENT_OUT_OF_RANGE, std::format("size constraint failed: !(min={} <= size={} <= max={})", Min, size, Max));
        }
    };

    template<auto Min, auto Max>
    inline static constexpr auto Range { RangeConstraint<Min, Max>{} };

    template<auto Min>
    inline static constexpr auto AtLeast { RangeConstraint<Min, std::numeric_limits<decltype(Min)>::max()>{} };

    template<auto Max>
    inline static constexpr auto AtMost { RangeConstraint<std::numeric_limits<decltype(Max)>::lowest(), Max>{} };

    template<size_t Min, size_t Max>
    inline static constexpr auto SizeRange { SizeRangeConstraint<Min, Max>{} };

    template<size_t Min>
    inline static constexpr auto SizeAtLeast { SizeRangeConstraint<Min, std::numeric_limits<size_t>::max()>{} };

    template<size_t Max>
    inline static constexpr auto SizeAtMost { SizeRangeConstraint<std::numeric_limits<size_t>::min(), Max>{} };

    template<size_t N>
    inline static constexpr auto SizeExactly { SizeRangeConstraint<N, N>{} };

    inline static constexpr auto NotEmpty { SizeAtLeast<1> };
}
