#pragma once

#include <functional>
#include <string_view>

#include "struo/forward.hpp"
#include "struo/concepts.hpp"

#include "struo/detail/Node.hpp"
#include "struo/detail/StrongAlias.hpp"

namespace struo {

using TagKey = detail::TaggedAlias<std::string_view, struct TagTag>;
using ContentKey = detail::TaggedAlias<std::string_view, struct TagContent>;

template<typename T>
requires std::is_enum_v<T> || IsStringLike<T>
[[nodiscard]] constexpr std::string_view as_string(T value) {
    if constexpr (std::is_enum_v<T>) {
        return enum_name(value);
    } else {
        return value;
    }
}

template <auto Tag, typename T>
struct Bind {
    static constexpr std::string_view tag = as_string(Tag);
    using value_type = T;
};

template<typename... Bs>
using Bindings = detail::TaggedArgPack<struct TagBindings, Bs...>;

inline constexpr TagKey DefaultTagKey { TagKey { "type" } };
inline constexpr ContentKey DefaultContentKey { ContentKey { "value" } };

template <typename... Bs>
class Variant {
public:
    template <typename... Args>
    constexpr explicit Variant(Bindings<Bs...> bindings, Args&&... args) : bindings_{std::move(bindings)}  {
        (apply(std::forward<Args>(args)), ...);
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
