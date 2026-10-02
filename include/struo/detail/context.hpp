#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <type_traits>
#include <algorithm>
#include <ranges>
#include <vector>
#include <format>

#include "struo/concepts.hpp"
#include "struo/Result.hpp"
#include "struo/detail/detail.hpp"
#include "struo/detail/traits.hpp"

namespace struo::detail {

    template<typename... Callables>
    requires (std::invocable<Callables&> && ...)
    class ScopedGuard {
    public:
        constexpr ScopedGuard(Callables... callable) : callables_{std::move(callable)...} {}

        constexpr ~ScopedGuard() {
            if(!active_) {
                return;
            }
            std::apply([](auto&... callbacks){
                (std::invoke(callbacks), ...);
            }, callables_);
        }

        constexpr ScopedGuard(ScopedGuard&& other) noexcept(std::is_nothrow_move_constructible_v<std::tuple<Callables...>>)
            : callables_{std::move(other.callables_)}, active_{std::exchange(other.active_, false)} {}

        ScopedGuard& operator=(ScopedGuard&&) = delete;
        ScopedGuard& operator=(const ScopedGuard&) = delete;
        ScopedGuard(const ScopedGuard&) = delete;

    private:
        std::tuple<Callables...> callables_;
        bool active_{true};
    };

    template<typename... Callables>
    [[nodiscard]] constexpr auto scoped(Callables&&... callables) {
        return ScopedGuard { std::forward<Callables>(callables)... };
    }

    template<typename T>
    [[nodiscard]] constexpr std::string format_key(const T& t) {
        if constexpr (IsStringLike<T>) {
            return std::format("\"{}\"", std::string_view{t});
        } else {
            return std::format("{}", t);
        }
    }

    [[nodiscard]] constexpr std::string format_field_name(std::string_view name) {
        if(name.find_first_of("[].\"'") != std::string_view::npos) {
            return std::format("\"{}\"", name);
        }
        return std::string{name};
    }

    template<typename... SubContexts>
    class Context {
    public:
        Context() = default;
        ~Context() = default;

        Context(Context&&) = delete;
        Context& operator=(Context&&) = delete;
        Context& operator=(const Context&) = delete;
        Context(const Context&) = delete;

        [[nodiscard]] constexpr auto enterObject() {
            return notifyAll([](auto& context) { return context.enterObject(); });
        }

        [[nodiscard]] constexpr auto enterField(std::string_view key) {
            return notifyAll([&](auto& context) { return context.enterField(key); });
        }

        template<typename Key>
        [[nodiscard]] constexpr auto enterMember(const Key& key) {
            return notifyAll([&](auto& context) { return context.template enterMember<Key>(key); });
        }

        [[nodiscard]] constexpr auto enterElement(size_t index) {
            return notifyAll([&](auto& context) { return context.enterElement(index); });
        }

        [[nodiscard]] constexpr auto enterVariant(std::string_view binding) {
            return notifyAll([&](auto& context) { return context.enterVariant(binding); });
        }

        template<typename T>
        requires IsOneOf<T, SubContexts...>
        [[nodiscard]] constexpr const T& getSubcontext() const noexcept {
            return std::get<T>(subcontexts_);
        }

        template<typename T>
        requires IsOneOf<T, SubContexts...>
        [[nodiscard]] constexpr T& getSubcontext() noexcept {
            return std::get<T>(subcontexts_);
        }

    private:
        template<typename Callable>
        [[nodiscard]] constexpr auto notifyAll(Callable&& callable) {
            return std::apply([&](auto&... subcontexts){
                return std::tuple{std::invoke(callable, subcontexts)...};
            }, subcontexts_);
        }

    private:
        std::tuple<SubContexts...> subcontexts_;
    };

    class TraversalContext {
    public:
        TraversalContext() = default;
        ~TraversalContext() = default;

        TraversalContext(TraversalContext&&) = delete;
        TraversalContext& operator=(TraversalContext&&) = delete;
        TraversalContext& operator=(const TraversalContext&) = delete;
        TraversalContext(const TraversalContext&) = delete;

        [[nodiscard]] auto enterObject() {
            static constexpr auto no_op = [](){};
            return scoped(no_op);
        }

        [[nodiscard]] auto enterField(std::string_view key) {
            stack_.emplace_back(SegmentType::Field, format_field_name(key));
            return scoped([this] { exitLast(); });
        }

        template<typename Key>
        [[nodiscard]] auto enterMember(const Key& key) {
            stack_.emplace_back(SegmentType::MapKey, format_key(key));
            return scoped([this] { exitLast(); });
        }

        [[nodiscard]] auto enterElement(size_t index) {
            stack_.emplace_back(SegmentType::Index, std::format("{}", index));
            return scoped([this] { exitLast(); });
        }

        [[nodiscard]] auto enterVariant(std::string_view binding) {
            stack_.emplace_back(SegmentType::Variant, std::string{binding});
            return scoped([this] { exitLast(); });
        }

        [[nodiscard]] constexpr std::string getPath() const {
            std::string path{};
            for(const auto& segment: stack_) {
                if(segment.type == SegmentType::Field) {
                    if(!path.empty()) {
                        path += ".";
                    }
                    path += segment.text;
                } else if(segment.type == SegmentType::Variant) {
                    path += std::format("<{}>", segment.text);
                } else {
                    path += std::format("[{}]", segment.text);
                }
            }
            return path;
        }

    private:
        enum class SegmentType {
            Field,
            Index,
            MapKey,
            Variant
        };

        struct Segment {
            SegmentType type{};
            std::string text{};
        };

    private:
        void exitLast() {
            stack_.pop_back();
        }

    private:
        std::vector<Segment> stack_{};
    };

    [[nodiscard]] constexpr Error append_to_err(const Error& error, const TraversalContext& context) {
        if(auto path = context.getPath(); !path.empty()) {
            return err(error.code(), std::format("{}: {}", path, error.what()), error.where());
        }
        return error;
    }

}
