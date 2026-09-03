#pragma once

#include <tuple>
#include <expected>
#include <functional>

#include "struo/forward.hpp"
#include "struo/concepts.hpp"
#include "struo/Result.hpp"
#include "struo/Error.hpp"

namespace struo {

    template<typename... Fields>
    class Object {
    public:
        explicit constexpr Object(Fields... fields) : fields_{std::move(fields)...} {}

        template<typename Callable>
        requires (HasFunctionSignature<Callable, Result<void>(Fields&)> && ...)
        constexpr Result<void> forEachField(Callable&& callable) {
            Result<void> result { ok() };

            std::apply([&](auto&&... fields) {
                (... && [&]() {
                    result = std::invoke(callable, fields);
                    return result.ok();
                }());
            }, fields_);

            return result;
        }

        template<typename Callable>
        requires (HasFunctionSignature<Callable, Result<void>(const Fields&)> && ...)
        constexpr Result<void> forEachField(Callable&& callable) const {
            Result<void> result { ok() };

            std::apply([&](auto&&... fields) {
                (... && [&]() {
                    result = std::invoke(callable, fields);
                    return result.ok();
                }());
            }, fields_);

            return result;
        }

    private:
        std::tuple<Fields...> fields_;
    };

}
