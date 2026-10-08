#pragma once

#include <tuple>
#include <expected>
#include <functional>

#include "struo/forward.hpp"
#include "struo/Field.hpp"
#include "struo/concepts.hpp"
#include "struo/Result.hpp"

namespace struo {

/**
 * @brief Describes the configuration fields of a struct.
 *
 * Return an Object from SchemaTraits<Config>::schema() to define how the
 * struct is loaded.
 *
 * @code{.cpp}
 * struct Config {
 *     int port{};
 *     std::string name;
 * };
 *
 * template<>
 * struct struo::SchemaTraits<Config> {
 *     static auto schema() {
 *         return struo::Object{
 *             struo::Field<&Config::port>{struo::Keys{"port"}, struo::REQUIRED},
 *             struo::Field<&Config::name>{struo::Keys{"name"}}
 *         };
 *     }
 * };
 * @endcode
 */
template<typename... Fields>
requires (IsField<Fields> && ...)
class Object {
public:

    /**
     * @brief Constructs an object schema from its fields.
     * @param fields The Field definitions, processed in the supplied order.
     *
     * @code{.cpp}
     * Object{
     *     Field<&Config::port>{Keys{"port"}, REQUIRED},
     *     Field<&Config::name>{Keys{"name"}}
     * }
     * @endcode
     */
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
