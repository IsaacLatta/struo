#pragma once

#include <functional>
#include <format>
#include <string>
#include <string_view>

#include "struo/forward.hpp"
#include "struo/Result.hpp"
#include "struo/Error.hpp"
#include "struo/concepts.hpp"
#include "struo/detail/detail.hpp"

namespace struo {

    template<auto... Constraints>
    struct OrConstraint {
        static constexpr std::string_view name() {
            return "or";
        }

        constexpr Result<void> operator()(const auto& value) const {
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

        constexpr Result<void> operator()(const auto& value) const {
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

        constexpr Result<void> operator()(const auto& value) const {
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

        constexpr Result<void> operator()(const auto& value) const {
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
        requires IsSequence<T>
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
    inline constexpr auto Or { OrConstraint<Constraints...>{} };

    template<auto... Constraints>
    inline constexpr auto And { AndConstraint<Constraints...>{} };

    template<auto Constraint>
    inline constexpr auto Not { NotConstraint<Constraint>{} };

    template<auto... Constraints>
    inline constexpr auto ExactlyOne { ExactlyOneConstraint<Constraints...>{} };

    template<auto... Constraints>
    inline constexpr auto ForEach { ForEachConstraint<Constraints...>{} };
}
