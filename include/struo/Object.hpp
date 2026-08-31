#pragma once

#include <tuple>
#include <expected>
#include <functional>

#include "concepts.hpp"
#include "struo/Error.hpp"

namespace struo {

    template<typename... Fields>
    class Object {
    public:
        explicit constexpr Object(Fields... fields) : fields_{std::move(fields)...} {}

        template<typename Callable>
        requires (IsInvocable<Callable, std::expected<void, Error>(Fields&)> && ...)
        constexpr std::expected<void, Error> forEachField(Callable&& callable) {
            std::expected<void, Error> result{};

            std::apply([&](auto&&... fields) {
                (... && [&]() {
                    result = std::invoke(callable, fields);
                    return result.has_value();
                }());
            }, fields_);

            return result;
        }

    private:
        std::tuple<Fields...> fields_;
    };

}