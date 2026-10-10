#pragma once

#include <functional>
#include <string_view>
#include <array>
#include <type_traits>

#include "struo/forward.hpp"
#include "struo/concepts.hpp"
#include "struo/defaults.hpp"

#include "struo/detail/asserts.hpp"
#include "struo/detail/types.hpp"
#include "struo/detail/detail.hpp"

namespace struo {

/**
 * @brief Sets the key containing a variant's tag, which selects its alternative.
 *
 * Defaults to "type" when omitted from the Variant constructor.
 *
 * @code{.cpp}
 * Variant{Bindings{Bind<Kind::NUMBER, int>{}}, TagKey{"kind"}}
 * @endcode
 */
using TagKey = detail::TaggedAlias<std::string_view, struct TagTag>;

/**
 * @brief Sets the key containing the value of the selected variant alternative.
 *
 * Defaults to "value" when omitted from the Variant constructor.
 *
 * @code{.cpp}
 * Variant{Bindings{Bind<Kind::NUMBER, int>{}}, ContentKey{"data"}}
 * @endcode
 */
using ContentKey = detail::TaggedAlias<std::string_view, struct TagContent>;

/**
 * @brief Associates a configuration tag with a variant alternative's type.
 * @tparam Tag An enum value or a string-like constant identifying the alternative.
 * @tparam T The corresponding type in the destination std::variant.
 *
 * Enum tags use the enumerator's name, such as "NUMBER" or "number" for Kind::NUMBER.
 * Tags are compared case-sensitively and must be unique within a Variant schema.
 *
 * @code{.cpp}
 * Bindings{Bind<Kind::NUMBER, int>{}, Bind<Kind::TEXT, std::string>{}}
 * @endcode
 */
template <auto Tag, typename T>
requires (std::is_enum_v<decltype(Tag)> || IsStringLike<decltype(Tag)>) && (!IsPointerLike<decltype(Tag)> || Tag != nullptr)
struct Bind {
    using value_type = T;
    static constexpr std::string_view tag = detail::as_string(Tag);
};

/**
 * @brief Groups the tag-to-type bindings supplied to a Variant schema.
 *
 * Binding order does not need to match the destination std::variant's order.
 *
 * @code{.cpp}
 * Variant{Bindings{Bind<Kind::NUMBER, int>{}, Bind<Kind::TEXT, std::string>{}}}
 * @endcode
 */
template<typename... Args>
struct Bindings {
    std::tuple<Args...> values;
    constexpr explicit Bindings(Args... args) : values{std::move(args)...} {}
};


/** @brief The default variant tag key. */
inline constexpr TagKey DefaultTagKey { TagKey { "type" } };

/** @brief The default variant content key. */
inline constexpr ContentKey DefaultContentKey { ContentKey { "value" } };

/**
 * @brief Describes how a tagged configuration value is loaded into a std::variant.
 *
 * Return a Variant from the std::variant's SchemaTraits specialization. The tag
 * selects a binding, and the content is loaded as that binding's type. Both keys
 * are required. An unknown tag or invalid content fails loading.
 *
 * @code{.cpp}
 * enum class Kind { NUMBER, TEXT };
 * using Choice = std::variant<int, std::string>;
 *
 * template<>
 * struct struo::SchemaTraits<Choice> {
 *     static auto schema() {
 *         return struo::Variant{struo::Bindings{
 *             struo::Bind<Kind::NUMBER, int>{},
 *             struo::Bind<Kind::TEXT, std::string>{}
 *         }};
 *     }
 * };
 * @endcode
 *
 * A field of type Choice can then contain this YAML value:
 * @code{.yaml}
 * type: NUMBER
 * value: 1337
 * @endcode
 */
template <typename... Bs>
requires (sizeof...(Bs) > 0) && (IsBinding<Bs> && ...)
class Variant {
public:
    static_assert(detail::all_unique_binding_tags<Bs...>(), "binding tags must all be unique!");

public:
    using bindings_type = std::tuple<Bs...>;

public:
    /**
     * @brief Constructs a variant schema from its bindings and optional key settings.
     * @param bindings One or more bindings with unique tags and types belonging
     *                 to the destination std::variant.
     * @param args Optional TagKey and ContentKey settings, in either order.
     * @pre The tag and content keys must differ.
     *
     * The keys default to "type" and "value". Each key setting can be supplied
     * at most once.
     *
     * @code{.cpp}
     * Variant{
     *     Bindings{Bind<Kind::NUMBER, int>{}, Bind<Kind::TEXT, std::string>{}},
     *     TagKey{"kind"},
     *     ContentKey{"data"}
     * }
     * @endcode
     */
    template <typename... Args>
    requires (IsOneOf<Args, TagKey, ContentKey> && ...) && AllAppearAtMostOnce<Args...>
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
