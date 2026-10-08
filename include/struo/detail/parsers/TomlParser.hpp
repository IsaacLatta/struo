#pragma once

#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <toml++/toml.hpp>
#include <magic_enum/magic_enum.hpp>

#include "struo/Result.hpp"
#include "struo/concepts.hpp"
#include "struo/detail/detail.hpp"

namespace struo::detail {

    class TomlParser {
    public:
        explicit TomlParser(const toml::node& node) : node_(&node) {}

        template<typename T>
        requires std::is_enum_v<T>
        [[nodiscard]] Result<T> getAs() const {
            return detail::get_as_enum<T, TomlParser>(*this);
        }

        template<typename T>
        [[nodiscard]] Result<T> getAs() const {
            if(!node_) {
                if constexpr (detail::IsStringLike<T>) {
                    return T{key_};
                } else {
                    return err(INVALID_VALUE, "table key cannot be converted to the requested type");
                }
            }

            if(!node_->is_value()) {
                return wrongType("scalar");
            }

            if(auto value = node_->template value<T>()) {
                return std::move(*value);
            }

            return err(INVALID_VALUE, "value cannot be converted to the requested type");
        }

        template<typename T>
        requires IsChronoDuration<T>
        [[nodiscard]] Result<T> getAs() const {
            return detail::get_as_chrono_duration<T, TomlParser>(*this);
        }

        template<typename T>
        requires std::same_as<T, std::filesystem::path>
        [[nodiscard]] Result<T> getAs() const {
            return detail::get_as_path<TomlParser>(*this);
        }

        [[nodiscard]] Result<std::optional<TomlParser>> toChild(std::string_view name) const {
            const auto* table = node_ ? node_->as_table() : nullptr;
            if(!table) {
                return wrongType("table");
            }

            if(const auto* child = table->get(name)) {
                return std::optional<TomlParser>{TomlParser{*child}};
            }

            return std::optional<TomlParser>{};
        }

        [[nodiscard]] Result<std::vector<std::pair<TomlParser, TomlParser>>> getMembers() const {
            const auto* table = node_ ? node_->as_table() : nullptr;
            if(!table) {
                return wrongType("table");
            }

            std::vector<std::pair<TomlParser, TomlParser>> parsers{};
            parsers.reserve(table->size());
            for(const auto& [key, value] : *table) {
                parsers.emplace_back(TomlParser{key.str()}, TomlParser{value});
            }

            return parsers;
        }

        [[nodiscard]] Result<std::vector<TomlParser>> getElements() const {
            const auto* array = node_ ? node_->as_array() : nullptr;
            if(!array) {
                return wrongType("array");
            }
            
            std::vector<TomlParser> parsers{};
            parsers.reserve(array->size());
            for(const auto& value : *array) {
                parsers.emplace_back(value);
            }

            return parsers;
        }

    private:
        explicit TomlParser(std::string_view key) : key_(key) {}

        [[nodiscard]] Error wrongType(std::string_view expected) const {
            return err(WRONG_TYPE, std::format("expected {}, got {}", expected, node_ ? detail::enum_name(node_->type()) : "string"));
        }

        const toml::node* node_{};
        std::string_view key_{};
    };

} // namespace struo::detail
