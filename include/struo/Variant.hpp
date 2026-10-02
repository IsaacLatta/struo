#pragma once

#include <functional>
#include <string_view>
#include <array>

#include "struo/forward.hpp"
#include "struo/concepts.hpp"
#include "struo/defaults.hpp"

#include "struo/detail/asserts.hpp"
#include "struo/detail/Node.hpp"
#include "struo/detail/types.hpp"
#include "struo/detail/detail.hpp"

namespace struo {

using TagKey = detail::TaggedAlias<std::string_view, struct TagTag>;
using ContentKey = detail::TaggedAlias<std::string_view, struct TagContent>;

template <auto Tag, typename T, auto Definitions = Defines<>>
struct Bind {
    using value_type = T;
    using definitions_type = std::remove_cvref_t<decltype(Definitions)>;

    static constexpr std::string_view tag = detail::as_string(Tag);
    static constexpr auto domain_ids = detail::domain_ids_of(Definitions);
};

template<typename... Bs>
using Bindings = detail::TaggedArgPack<struct TagBindings, Bs...>;

inline constexpr TagKey DefaultTagKey { TagKey { "type" } };
inline constexpr ContentKey DefaultContentKey { ContentKey { "value" } };

template <typename... Bs>
class Variant {
public:
    static_assert(detail::all_unique_binding_tags<Bs...>(), "binding tags must all be unique!");

public:
    using bindings_type = std::tuple<Bs...>;

public:
    template <typename... Args>
    constexpr explicit Variant(Bindings<Bs...> bindings, Args&&... args) : bindings_{std::move(bindings)}  {
        (apply(std::forward<Args>(args)), ...);
        STRUO_ASSERT(tag_.value != content_.value, "variant tag and content keys must not match!");
    }

    [[nodiscard]] constexpr std::string_view getTagKey() const noexcept {
        return tag_.value;
    }

    [[nodiscard]] constexpr std::string_view getContentKey() const noexcept {
        return content_.value;
    }

    [[nodiscard]] constexpr const auto& getBindings() const noexcept {
        return bindings_.values;
    }

private:
    constexpr void apply(TagKey key) {
        tag_ = key;
    }

    constexpr void apply(ContentKey key) {
        content_ = key;
    }

private:
    TagKey tag_ { DefaultTagKey };
    ContentKey content_ { DefaultContentKey };
    Bindings<Bs...> bindings_;
};

} // namespace struo
