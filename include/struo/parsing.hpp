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

using Yaml = detail::YamlParser;
using Json = detail::JsonParser;
using Toml = detail::TomlParser;

template<typename UserObject, typename Parser>
requires HasSchema<UserObject> && IsOneOf<Parser, Yaml, Json, Toml>
constexpr Result<UserObject> load(Parser&& parser) {
    auto parsed = detail::parse<UserObject>(std::forward<Parser>(parser));
    if(!parsed) {
        return err(parsed);
    }
    return detail::materialize<UserObject>(*parsed);
}

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
