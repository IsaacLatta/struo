#pragma once

#include <utility>

#include "struo/forward.hpp"
#include "struo/Result.hpp"

#include "struo/detail/parsing_impl.hpp"
#include "struo/detail/materialization.hpp"
#include "struo/detail/file_loading.hpp"

#include "struo/parsing/JsonParser.hpp"
#include "struo/parsing/YamlParser.hpp"

namespace struo {

    template<typename UserObject, typename Parser>
    constexpr Result<UserObject> load(Parser&& parser) {
        auto parsed = detail::parse<UserObject>(std::forward<Parser>(parser));
        if(!parsed) {
            return err(parsed);
        }
        return detail::materialize<UserObject>(*parsed);
    }

    template<typename UserObject, detail::HasFormatReader Format>
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
