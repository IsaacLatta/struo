#pragma once

#include <format>
#include <string>
#include <cmath>
#include <string_view>
#include <type_traits>

#include "struo/concepts.hpp"
#include "struo/types.hpp"
#include "struo/Result.hpp"

namespace struo::detail {

    template<typename T>
    concept HasFormatter = std::is_default_constructible_v<std::formatter<std::remove_cvref_t<T>, char>>;

    template<auto... Values>
    struct OneOfConstraint {
        static constexpr std::string_view name() {
            return "one of";
        }

        static constexpr std::string description() {
            constexpr bool all_formattable = (HasFormatter<std::remove_cvref_t<decltype(Values)>> && ...);
            if constexpr (all_formattable) {
                return std::format("expects one of: {}", formatValues());
            } else {
                return "expects one of a set of values";
            }
        }

        constexpr Result<void> operator()(const auto& t) const {
            const auto compare = [&](const auto& val) {
                return !(val == t);
            };

            const bool no_matches = (compare(Values) && ...);
            if(no_matches) {
                return err(ARGUMENT_OUT_OF_RANGE, std::format("\"{}\" constraint failed: {}", name(), description()));
            }

            return ok();
        }

    private:
        static constexpr std::string formatValues() {
            std::string out{};
            auto format_one = [&](const auto& val) {
                out += out.empty() ? std::format("{}", val) : std::format(", {}", val);
            };

            (format_one(Values), ...);
            return out;
        }
    };

    template<Str Prefix>
    struct StartsWithConstraint {
        static constexpr std::string_view name() {
            return "starts with";
        }

        static constexpr std::string description() {
            return std::format("expects prefix \"{}\"", Prefix.string);
        }

        constexpr Result<void> operator()(const std::string& str) const {
            if(str.starts_with(std::string_view{Prefix.string})) {
                return ok();
            }
            return err(INVALID_ARGUMENT, std::format("\"{}\" constraint failed: {}", name(), description()));
        }
    };

    template<Str PostFix>
    struct EndsWithConstraint {
        static constexpr std::string_view name() {
            return "ends with";
        }

        static constexpr std::string description() {
            return std::format("expects postfix \"{}\"", PostFix.string);
        }

        constexpr Result<void> operator()(const std::string& str) const {
            if(str.ends_with(std::string_view{PostFix.string})) {
                return ok();
            }
            return err(INVALID_ARGUMENT, std::format("\"{}\" constraint failed: {}", name(), description()));
        }
    };

    struct IsFiniteConstraint {
        static constexpr std::string_view name() {
            return "is finite";
        }

        template<typename T>
        constexpr Result<void> operator()(const T& value) const {
            if constexpr (std::is_floating_point_v<T>) {
                if(!std::isfinite(value)) {
                    return err(INVALID_ARGUMENT, std::format("\"{}\" failed, {:.3f}... is not finite", name(), value));
                }
                return ok();
            } else {
                return ok();
            }
        }
    };

    struct PositiveConstraint {
        static constexpr std::string_view name() {
            return "positive";
        }

        template<typename T>
        constexpr Result<void> operator()(const T& t) const {
            if constexpr (std::is_floating_point_v<T>) {
                if(t > 0.0f) {
                    return ok();
                }
                return err(ARGUMENT_OUT_OF_RANGE, std::format("\"{}\" constraint failed: !({:.2f} > 0.0)", name(), t));
            } else {
                if(t > 0) {
                    return ok();
                }
                return err(ARGUMENT_OUT_OF_RANGE, std::format("\"{}\" constraint failed: !({} > 0)", name(), t));
            }
        }
    };

    struct NotEmptyConstraint {
        static constexpr std::string_view name() {
            return "not empty";
        }

        template<typename T>
        requires HasEmptyApi<T>
        Result<void> operator()(const T& container) const {
            if(container.empty()) {
                return err(INVALID_VALUE, std::format("\"{}\" constraint failed", name()));
            }
            return ok();
        }
    };
}
