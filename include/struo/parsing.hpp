#pragma once

#include <filesystem>
#include <utility>

#include "struo/concepts.hpp"
#include "struo/forward.hpp"
#include "struo/Result.hpp"

#include "struo/detail/parsing_impl.hpp"
#include "struo/detail/materialization.hpp"
#include "struo/detail/file_loading.hpp"

#include "struo/detail/parsers/JsonParser.hpp"
#include "struo/detail/parsers/YamlParser.hpp"
#include "struo/detail/parsers/TomlParser.hpp"

namespace struo {

/**
 * @brief Selects YAML configuration files.
 *
 * @code{.cpp}
 * auto result = struo::load<Config, struo::Yaml>("config.yaml");
 * @endcode
 */
using Yaml = detail::YamlParser;

/**
 * @brief Selects JSON configuration files.
 *
 * @code{.cpp}
 * auto result = struo::load<Config, struo::Json>("config.json");
 * @endcode
 */
using Json = detail::JsonParser;

/**
 * @brief Selects TOML configuration files.
 *
 * @code{.cpp}
 * auto result = struo::load<Config, struo::Toml>("config.toml");
 * @endcode
 */
using Toml = detail::TomlParser;

/**
 * @brief Loads a configuration from an existing parser.
 * @tparam UserObject The configuration type, with a SchemaTraits specialization.
 * @tparam Parser A Yaml, Json, or Toml parser; deduced from the argument.
 * @param parser The parser for the configuration document or node.
 * @return The loaded configuration, or an error if conversion, field resolution,
 *         a default, a transform, or a constraint fails.
 *
 * Applies the schema's defaults, transforms, and constraints. Keep any document
 * referenced by the parser alive until loading finishes. Errors from parsing
 * the document's native syntax before this call are handled by the format library.
 *
 * @code{.cpp}
 * auto document = YAML::Load("port: 8080");
 * auto result = struo::load<Config>(struo::Yaml{document});
 * if (result) {
 *     use_config(*result);
 * } else {
 *     handle_error(result.error());
 * }
 * @endcode
 */
template<typename UserObject, typename Parser>
requires HasSchema<UserObject> && IsOneOf<Parser, Yaml, Json, Toml>
constexpr Result<UserObject> load(Parser&& parser) {
    auto parsed = detail::parse<UserObject>(std::forward<Parser>(parser));
    if(!parsed) {
        return err(parsed);
    }
    return detail::materialize<UserObject>(*parsed);
}

/**
 * @brief Reads a configuration file and loads it using the selected format.
 * @tparam UserObject The configuration type, with a SchemaTraits specialization.
 * @tparam Format The file format: Yaml, Json, or Toml.
 * @param path The configuration file to read.
 * @return The loaded configuration, or an error if reading, parsing, or schema
 *         processing fails.
 *
 * The format is selected explicitly, rather than inferred from the file extension.
 * Applies the schema's defaults, transforms, and constraints. Relative file paths
 * are resolved against the current working directory; paths inside the configuration
 * are not automatically resolved against the configuration file's directory.
 *
 * @code{.cpp}
 * auto result = struo::load<Config, struo::Yaml>("config.yaml");
 * if (result) {
 *     use_config(*result);
 * } else {
 *     handle_error(result.error());
 * }
 * @endcode
 */
template<typename UserObject, typename Format>
requires HasSchema<UserObject> && IsOneOf<Format, Yaml, Json, Toml>
[[nodiscard]] Result<UserObject> load(const std::filesystem::path& path) {
    auto contents = detail::read_file(path);
    if (!contents) {
        return err(contents);
    }

    auto document = detail::FormatReader<Format>::parse(*contents);
    if (!document) {
        return err(document);
    }

    auto parser = detail::FormatReader<Format>::makeParser(*document);
    return load<UserObject>(parser);
}

}
