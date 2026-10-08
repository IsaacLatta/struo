#pragma once

#include <format>
#include <functional>
#include <filesystem>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "struo/Result.hpp"
#include "struo/concepts.hpp"
#include "struo/detail/detail.hpp"

namespace struo::detail {

class JsonParser {
public:
    explicit JsonParser(nlohmann::json node) : node_(std::move(node)) {}

    template<typename T>
    requires (!std::is_enum_v<T> && !IsChronoDuration<T> && !std::same_as<T, std::filesystem::path>)
    [[nodiscard]] Result<T> getAs() const {
        if(!node_.is_string() && !node_.is_boolean() && !node_.is_number()) {
            return err(WRONG_TYPE, std::format("expected scalar, got {}", node_.type_name()));
        }

        return tryParse<T>([this] { return node_.template get<T>(); });
    }

    template<typename T>
    requires IsChronoDuration<T>
    [[nodiscard]] Result<T> getAs() const {
        return detail::get_as_chrono_duration<T, JsonParser>(*this);
    }

    template<typename T>
    requires std::same_as<T, std::filesystem::path>
    [[nodiscard]] Result<std::filesystem::path> getAs() const {
        return detail::get_as_path<JsonParser>(*this);
    }

    template<typename T>
    requires std::is_enum_v<T>
    [[nodiscard]] Result<T> getAs() const {
        return detail::get_as_enum<T, JsonParser>(*this);
    }

    [[nodiscard]] Result<std::optional<JsonParser>> toChild(std::string_view name) const {
        return tryParse<std::optional<JsonParser>>([&]() -> std::optional<JsonParser> {
            if(const auto child = node_.find(name); child != node_.end()) {
                return JsonParser{*child};
            }

            return std::nullopt;
        }, nlohmann::json::value_t::object);
    }

    [[nodiscard]] Result<std::vector<std::pair<JsonParser, JsonParser>>> getMembers() const {
        return tryParse<std::vector<std::pair<JsonParser, JsonParser>>>([this] {
            std::vector<std::pair<JsonParser, JsonParser>> parsers{};
            parsers.reserve(node_.size());
            for(auto it = node_.begin(); it != node_.end(); ++it) {
                parsers.emplace_back(JsonParser{nlohmann::json(it.key())}, JsonParser{it.value()});
            }
            return parsers;
        }, nlohmann::json::value_t::object);
    }

    [[nodiscard]] Result<std::vector<JsonParser>> getElements() const {
        return tryParse<std::vector<JsonParser>>([this] {
            std::vector<JsonParser> parsers{};
            parsers.reserve(node_.size());
            for(const auto& element : node_) {
                parsers.emplace_back(element);
            }
            return parsers;
        }, nlohmann::json::value_t::array);
    }

private:
    template<typename T, typename Callable>
    requires HasFunctionSignature<Callable, T()>
    [[nodiscard]] Result<T> tryParse(Callable&& callable, std::optional<nlohmann::json::value_t> expected_type = std::nullopt) const {
        try {
            if(expected_type && node_.type() != *expected_type) {
                return err(WRONG_TYPE, std::format("expected {}, got {}",
                    nlohmann::json(*expected_type).type_name(), node_.type_name()));
            }
            return std::invoke(std::forward<Callable>(callable));
        } catch(const std::filesystem::filesystem_error& e) {
            return err(INVALID_VALUE, e.what());
        } catch(const nlohmann::json::type_error& e) {
            return err(INVALID_VALUE, e.what());
        } catch(const nlohmann::json::out_of_range& e) {
            return err(INVALID_VALUE, e.what());
        } catch(const nlohmann::json::exception& e) {
            return err(PARSE_ERROR, e.what());
        }
    }

private:
    nlohmann::json node_;
};

}
