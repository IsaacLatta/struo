#pragma once

#include <functional>

#include "struo/forward.hpp"
#include "struo/Result.hpp"
#include "struo/Error.hpp"
#include "struo/concepts.hpp"

namespace struo {

    template<auto... Constraints>
    struct OrConstraint {
        constexpr Result<void> operator()(const auto& value) const {
            const bool any_succeeded = (std::invoke(Constraints, value).ok() || ...);
            if(!any_succeeded) {
                return err(INVALID_ARGUMENT, "\"or\" constraint failure");
            }
            return ok();
        }
    };

    template<auto... Constraints>
    struct AndConstraint {
        constexpr Result<void> operator()(const auto& value) const {
            const bool all_succeeded = (std::invoke(Constraints, value).ok() && ...);
            if(!all_succeeded) {
                return err(INVALID_ARGUMENT, "\"and\" constraint failure");
            }
            return ok();
        }
    };

    template<auto Constraint>
    struct NotConstraint {
        constexpr Result<void> operator()(const auto& value) const {
            if(auto result = std::invoke(Constraint, value); !result.ok()) {
                return ok();
            }
            return err(INVALID_ARGUMENT, "\"not\" constraint failure");
        }
    };

    template<auto... Constraints>
    struct ExactlyOneConstraint {
        constexpr Result<void> operator()(const auto& value) const {
            size_t n_succeeded { 0u };
            auto invoke_one = [&](const auto& constraint) -> bool {
                if(const auto result = std::invoke(constraint, value); result.ok()) {
                    ++n_succeeded;
                }
                return n_succeeded < 2u;
            };

            (invoke_one(Constraints) && ...);
            if(n_succeeded != 1) {
                return err(INVALID_ARGUMENT, std::format("\"exactly one\" constraint failed (n={} succeeded)", n_succeeded));
            }

            return ok();
        }
    };

    template<auto... Constraints>
    struct ForEachConstraint {
        template<typename T>
        requires IsSequence<T>
        constexpr Result<void> operator()(const T& value) const {
            Result<void> final_result { ok() };

            auto invoke_one = [&](const auto& constraint, const auto& element){
                if(auto result = std::invoke(constraint, element); !result) {
                    final_result = std::move(result);
                    return false;
                }
                return true;
            };

            size_t i { 0u };
            for(const auto& element: value) {
                (invoke_one(Constraints, element) && ...);
                if(!final_result) {
                    return err(final_result.error().code(),
                        std::format("\"for each\" constraint failed: {} (on index={})", final_result.error().what(), i));
                }
                ++i;
            }

            return ok();
        }
    };

    template<auto... Constraints>
    static inline constexpr auto Or { OrConstraint<Constraints...>{} };

    template<auto... Constraints>
    static inline constexpr auto And { AndConstraint<Constraints...>{} };

    template<auto Constraint>
    static inline constexpr auto Not { NotConstraint<Constraint>{} };

    template<auto... Constraints>
    static inline constexpr auto ForEach { ForEachConstraint<Constraints...>{} };
}
