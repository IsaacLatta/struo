#pragma once

#include <functional>
#include <ranges>
#include <format>
#include <string>
#include <string_view>

#include "struo/forward.hpp"
#include "struo/Result.hpp"
#include "struo/concepts.hpp"

#include "struo/detail/detail.hpp"

namespace struo::detail {

template<auto... Constraints>
struct OrConstraint {
    static constexpr std::string_view name() {
        return "or";
    }

    template<typename T>
    requires (IsConstraintFor<decltype((Constraints)), T> && ...)
    constexpr Result<void> operator()(const T& value) const {
        std::string failures;
        auto invoke_one = [&](const auto& constraint) {
            auto result = std::invoke(constraint, value);
            if(result) {
                return true;
            }
            if(!failures.empty()) {
                failures += "; ";
            }
            failures += result.error().what();
            return false;
        };
        const bool any_succeeded = (invoke_one(Constraints) || ...);
        if(!any_succeeded) {
            return err(INVALID_ARGUMENT, std::format("\"{}\" constraint failed: no constraints matched{}{}",
                name(), failures.empty() ? "" : "; ", failures));
        }
        return ok();
    }
};

template<auto... Constraints>
struct AndConstraint {
    static constexpr std::string_view name() {
        return "and";
    }

    template<typename T>
    requires (IsConstraintFor<decltype((Constraints)), T> && ...)
    constexpr Result<void> operator()(const T& value) const {
        Result<void> final_result { ok() };
        auto invoke_one = [&](const auto& constraint) {
            if(auto result = std::invoke(constraint, value); !result) {
                final_result = err(INVALID_ARGUMENT, std::format("\"{}\" constraint failed: {}",
                    name(), result.error().what()));
                return false;
            }
            return true;
        };
        (invoke_one(Constraints) && ...);
        return final_result;
    }
};

template<auto Constraint>
struct NotConstraint {
    static constexpr std::string_view name() {
        return "not";
    }

    template<typename T>
    requires IsConstraintFor<decltype((Constraint)), T>
    constexpr Result<void> operator()(const T& value) const {
        if(auto result = std::invoke(Constraint, value); !result.ok()) {
            return ok();
        }
        return err(INVALID_ARGUMENT, std::format("\"{}\" constraint failed: \"{}\" unexpectedly matched",
            name(), detail::name_of<decltype(Constraint)>()));
    }
};

template<auto... Constraints>
struct ExactlyOneConstraint {
    static constexpr std::string_view name() {
        return "exactly one";
    }

    template<typename T>
    requires (IsConstraintFor<decltype((Constraints)), T> && ...)
    constexpr Result<void> operator()(const T& value) const {
        size_t n_succeeded { 0u };
        std::string failures;
        std::string matches;
        auto invoke_one = [&](const auto& constraint) -> bool {
            if(const auto result = std::invoke(constraint, value); result.ok()) {
                ++n_succeeded;
                if(!matches.empty()) {
                    matches += ", ";
                }
                matches += std::format("\"{}\"", detail::name_of<decltype(constraint)>());
            } else if(n_succeeded == 0u) {
                if(!failures.empty()) {
                    failures += "; ";
                }
                failures += result.error().what();
            }
            return n_succeeded < 2u;
        };

        (invoke_one(Constraints) && ...);
        if(n_succeeded == 0u) {
            return err(INVALID_ARGUMENT, std::format("\"{}\" constraint failed: no constraints matched{}{}",
                name(), failures.empty() ? "" : "; ", failures));
        }
        if(n_succeeded > 1u) {
            return err(INVALID_ARGUMENT, std::format("\"{}\" constraint failed: at least two constraints matched: {}",
                name(), matches));
        }

        return ok();
    }
};

template<auto... Constraints>
struct ForEachConstraint {
    static constexpr std::string_view name() {
        return "for each";
    }

    template<typename T>
    requires IsSequence<T> && (IsConstraintFor<decltype(Constraints), typename T::value_type> && ...)
    constexpr Result<void> operator()(const T& value) const {
        Result<void> final_result { ok() };
        size_t i { 0u };

        auto invoke_one = [&](const auto& constraint, const auto& element){
            if(auto result = std::invoke(constraint, element); !result) {
                final_result = err(result.error().code(),
                    std::format("\"{}\" constraint failed: {} (on index={})",
                        name(), result.error().what(), i));
                return false;
            }
            return true;
        };

        for(const auto& element: value) {
            (invoke_one(Constraints, element) && ...);
            if(!final_result) {
                return final_result;
            }
            ++i;
        }

        return ok();
    }
};

template<auto... Constraints>
struct IfPresentConstraintT {
public:
    template<typename T>
    requires (IsConstraintFor<decltype(Constraints), T> && ...)
    constexpr Result<void> operator()(const std::optional<T>& value) const {
        if(!value) {
            return ok();
        }

        Result<void> result { ok() };
        auto check_one = [&](const auto& constraint) {
            result = std::invoke(constraint, *value);
            return result.ok();
        };

        (void)(check_one(Constraints) && ...);
        return result;
    }
};

} // struo::detail
