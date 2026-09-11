#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <type_traits>
#include <vector>
#include <format>

#include "struo/concepts.hpp"
#include "struo/Error.hpp"
#include "struo/Result.hpp"
#include "struo/detail/detail.hpp"

namespace struo::detail {

    template<typename Callable>
    requires std::invocable<Callable&>
    class ScopedGuard {
    public:
        ScopedGuard(Callable callable) : callable_{std::move(callable)} {}

        ~ScopedGuard() { std::invoke(callable_); }

        ScopedGuard(ScopedGuard&&) = delete;
        ScopedGuard& operator=(ScopedGuard&&) = delete;
        ScopedGuard& operator=(const ScopedGuard&) = delete;
        ScopedGuard(const ScopedGuard&) = delete;

    private:
        Callable callable_;
    };

    [[nodiscard]] auto scoped(auto&& callable) {
        return ScopedGuard<std::remove_cvref_t<decltype(callable)>> { std::forward<decltype(callable)>(callable) };
    }

    template<typename T>
    concept IsStringLike = std::convertible_to<const T&, std::string_view>;

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

    class TraversalContext {
    public:
        TraversalContext() = default;
        ~TraversalContext() = default;

        TraversalContext(TraversalContext&&) = delete;
        TraversalContext& operator=(TraversalContext&&) = delete;
        TraversalContext& operator=(const TraversalContext&) = delete;
        TraversalContext(const TraversalContext&) = delete;

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

        [[nodiscard]] constexpr std::string getPath() const {
            std::string path{};
            for(const auto& segment: stack_) {
                if(segment.type == SegmentType::Field) {
                    if(!path.empty()) {
                        path += ".";
                    }
                    path += segment.text;
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
            MapKey
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
