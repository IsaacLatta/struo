#pragma once

#include <string_view>
#include <optional>
#include <functional>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "struo/forward.hpp"
#include "struo/concepts.hpp"
#include "struo/Result.hpp"

#include "struo/detail/detail.hpp"

namespace struo {

    class YamlParser {
    public:
        explicit YamlParser(YAML::Node node) : node_(std::move(node))  {}

        template<typename T>
        [[nodiscard]] Result<T> getAs() const {
            return tryParse<T>([this] {
                return node_.as<T>();
            }, YAML::NodeType::Scalar);
        }

        [[nodiscard]] Result<std::optional<YamlParser>> toChild(std::string_view name) const {
            return tryParse<std::optional<YamlParser>>([&]() -> std::optional<YamlParser> {
                const auto child = node_[name];
                if (!child) {
                    return std::nullopt;
                }
                return YamlParser { child };
            });
        }

        [[nodiscard]] Result<std::vector<std::pair<YamlParser, YamlParser>>> getMembers() const {
            return tryParse<std::vector<std::pair<YamlParser, YamlParser>>>([this] {
                std::vector<std::pair<YamlParser, YamlParser>> parsers{};
                for (auto it = node_.begin(); it != node_.end(); ++it) {
                    parsers.emplace_back(it->first, it->second);
                }
                return parsers;
            }, YAML::NodeType::Map);
        }

        [[nodiscard]] Result<std::vector<YamlParser>> getElements() const {
            return tryParse<std::vector<YamlParser>>([this]() -> std::vector<YamlParser> {
                std::vector<YamlParser> parsers{};
                for (size_t i { 0 }; i < node_.size(); ++i) {
                    parsers.emplace_back(node_[i]);
                }
                return parsers;
            }, YAML::NodeType::Sequence);
        }

    private:
        template<typename T, typename Callable>
        requires HasFunctionSignature<Callable, T()>
        [[nodiscard]] Result<T> tryParse(Callable&& callable, std::optional<YAML::NodeType::value> expected_type = std::nullopt) const {
            try {
                if (expected_type) {
                    if (node_.Type() != *expected_type) {
                        return err(WRONG_TYPE, std::format("expected {}, got {}", detail::enum_name(*expected_type), detail::enum_name(node_.Type())));
                    }
                }
                return std::invoke(std::forward<Callable>(callable));
            } catch (const YAML::ParserException& e) {
                return err(SYNTAX_ERROR, e.what());
            } catch (const YAML::BadConversion& e) {
                return err(INVALID_VALUE, e.what());
            } catch (const YAML::BadSubscript& e) {
                return err(WRONG_TYPE, e.what());
            } catch (const YAML::Exception& e) {
                return err(PARSE_ERROR, e.what());
            }
        }

    private:
        YAML::Node node_{};

    };

    using Yaml = YamlParser;

}
